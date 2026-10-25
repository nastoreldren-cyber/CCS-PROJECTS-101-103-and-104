#define _CRT_SECURE_NO_WARNINGS  /* stops Visual Studio "unsafe function" errors */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FILE_NAME "donors.dat"
#define TEMP_NAME "temp.dat"
#define MIN_AGE 18
#define MAX_AGE 65
#define MIN_WEIGHT 50
#define MIN_GAP_DAYS 90
#define NEVER 9999

typedef struct {
    int id;
    char name[50];
    int age;
    float weight;
    char bloodGroup[4];        /* A+, A-, B+, B-, AB+, AB-, O+, O- */
    char phone[15];
    int daysSinceLastDonation; /* 9999 = never donated */
} Donor;

/* ---------- helpers ---------- */

void clearInput(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* Safe line reader: never overflows and never leaves extra text behind */
void readString(const char *prompt, char *dest, int size) {
    char buf[256];
    size_t len;

    printf("%s", prompt);
    dest[0] = '\0';
    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return;

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    else
        clearInput();               /* line was longer than buffer */

    strncpy(dest, buf, size - 1);
    dest[size - 1] = '\0';
}

void toUpperStr(char *s) {
    for (; *s; s++)
        *s = (char)toupper((unsigned char)*s);
}

int validBloodGroup(const char *bg) {
    const char *valid[] = {"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"};
    int i;
    for (i = 0; i < 8; i++)
        if (strcmp(bg, valid[i]) == 0)
            return 1;
    return 0;
}

int isEligible(const Donor *d) {
    return d->age >= MIN_AGE && d->age <= MAX_AGE &&
           d->weight >= MIN_WEIGHT &&
           d->daysSinceLastDonation >= MIN_GAP_DAYS;
}

int nextId(void) {
    FILE *fp;
    Donor d;
    int maxId = 0;

    fp = fopen(FILE_NAME, "rb");
    if (!fp) return 1;
    while (fread(&d, sizeof(Donor), 1, fp) == 1)
        if (d.id > maxId) maxId = d.id;
    fclose(fp);
    return maxId + 1;
}

void printHeader(void) {
    printf("\n%-5s %-20s %-4s %-7s %-6s %-14s %-12s %-9s\n",
           "ID", "Name", "Age", "Weight", "Blood", "Phone", "LastDonation", "Eligible");
    printf("-----------------------------------------------------------------------------------\n");
}

void printDonor(const Donor *d) {
    char last[32];
    if (d->daysSinceLastDonation == NEVER)
        strcpy(last, "Never");
    else
        sprintf(last, "%d days", d->daysSinceLastDonation);
    printf("%-5d %-20s %-4d %-7.1f %-6s %-14s %-12s %-9s\n",
           d->id, d->name, d->age, d->weight, d->bloodGroup,
           d->phone, last, isEligible(d) ? "Yes" : "No");
}

/* ---------- features ---------- */

void addDonor(void) {
    Donor d;
    FILE *fp;

    memset(&d, 0, sizeof(Donor));
    d.id = nextId();

    readString("Name: ", d.name, sizeof(d.name));

    printf("Age: ");
    if (scanf("%d", &d.age) != 1) { clearInput(); printf("Invalid age.\n"); return; }
    printf("Weight (kg): ");
    if (scanf("%f", &d.weight) != 1) { clearInput(); printf("Invalid weight.\n"); return; }
    clearInput();

    readString("Blood group (A+, A-, B+, B-, AB+, AB-, O+, O-): ", d.bloodGroup, sizeof(d.bloodGroup));
    toUpperStr(d.bloodGroup);
    if (!validBloodGroup(d.bloodGroup)) {
        printf("Invalid blood group.\n");
        return;
    }

    readString("Phone: ", d.phone, sizeof(d.phone));

    printf("Days since last donation (enter 9999 if never donated): ");
    if (scanf("%d", &d.daysSinceLastDonation) != 1) { clearInput(); printf("Invalid input.\n"); return; }
    clearInput();

    fp = fopen(FILE_NAME, "ab");
    if (!fp) { printf("Error opening file.\n"); return; }
    fwrite(&d, sizeof(Donor), 1, fp);
    fclose(fp);

    printf("\nDonor added with ID %d. Eligible to donate now: %s\n",
           d.id, isEligible(&d) ? "Yes" : "No");
}

void displayAll(void) {
    FILE *fp;
    Donor d;
    int count = 0;

    fp = fopen(FILE_NAME, "rb");
    if (!fp) { printf("No donor records found.\n"); return; }

    printHeader();
    while (fread(&d, sizeof(Donor), 1, fp) == 1) {
        printDonor(&d);
        count++;
    }
    fclose(fp);
    if (count == 0) printf("No donor records found.\n");
    else printf("\nTotal donors: %d\n", count);
}

void searchByBloodGroup(void) {
    char bg[4];
    Donor d;
    int found = 0;
    FILE *fp;

    readString("Enter blood group to search: ", bg, sizeof(bg));
    toUpperStr(bg);

    fp = fopen(FILE_NAME, "rb");
    if (!fp) { printf("No donor records found.\n"); return; }

    printHeader();
    while (fread(&d, sizeof(Donor), 1, fp) == 1) {
        if (strcmp(d.bloodGroup, bg) == 0) {
            printDonor(&d);
            found++;
        }
    }
    fclose(fp);
    if (!found) printf("No donors found with blood group %s.\n", bg);
}

void updateDonation(void) {
    int id, found = 0;
    Donor d;
    FILE *fp;

    printf("Enter donor ID: ");
    if (scanf("%d", &id) != 1) { clearInput(); printf("Invalid ID.\n"); return; }
    clearInput();

    fp = fopen(FILE_NAME, "rb+");
    if (!fp) { printf("No donor records found.\n"); return; }

    while (fread(&d, sizeof(Donor), 1, fp) == 1) {
        if (d.id == id) {
            d.daysSinceLastDonation = 0;  /* donated today */
            fseek(fp, -(long)sizeof(Donor), SEEK_CUR);
            fwrite(&d, sizeof(Donor), 1, fp);
            found = 1;
            break;
        }
    }
    fclose(fp);
    if (found) printf("Donation recorded for %s. Next eligible in %d days.\n", d.name, MIN_GAP_DAYS);
    else printf("Donor ID %d not found.\n", id);
}

void deleteDonor(void) {
    int id, found = 0;
    Donor d;
    FILE *fp, *tmp;

    printf("Enter donor ID to delete: ");
    if (scanf("%d", &id) != 1) { clearInput(); printf("Invalid ID.\n"); return; }
    clearInput();

    fp = fopen(FILE_NAME, "rb");
    if (!fp) { printf("No donor records found.\n"); return; }
    tmp = fopen(TEMP_NAME, "wb");
    if (!tmp) { printf("File error.\n"); fclose(fp); return; }

    while (fread(&d, sizeof(Donor), 1, fp) == 1) {
        if (d.id == id) found = 1;
        else fwrite(&d, sizeof(Donor), 1, tmp);
    }
    fclose(fp);
    fclose(tmp);

    if (found) {
        remove(FILE_NAME);
        rename(TEMP_NAME, FILE_NAME);
        printf("Donor deleted.\n");
    } else {
        remove(TEMP_NAME);
        printf("Donor ID %d not found.\n", id);
    }
}

void showEligible(void) {
    FILE *fp;
    Donor d;
    int count = 0;

    fp = fopen(FILE_NAME, "rb");
    if (!fp) { printf("No donor records found.\n"); return; }

    printHeader();
    while (fread(&d, sizeof(Donor), 1, fp) == 1) {
        if (isEligible(&d)) {
            printDonor(&d);
            count++;
        }
    }
    fclose(fp);
    if (!count) printf("No eligible donors right now.\n");
}

/* ---------- main ---------- */

int main(void) {
    int choice;

    do {
        printf("\n===== BLOOD DONATION MANAGEMENT =====\n");
        printf("1. Add new donor\n");
        printf("2. Display all donors\n");
        printf("3. Search donors by blood group\n");
        printf("4. Show eligible donors\n");
        printf("5. Record a new donation\n");
        printf("6. Delete donor\n");
        printf("0. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1)
            choice = -1;
        clearInput();

        switch (choice) {
            case 1: addDonor(); break;
            case 2: displayAll(); break;
            case 3: searchByBloodGroup(); break;
            case 4: showEligible(); break;
            case 5: updateDonation(); break;
            case 6: deleteDonor(); break;
            case 0: printf("Goodbye!\n"); break;
            default: printf("Invalid choice. Try again.\n");
        }
    } while (choice != 0);

    return 0;
}
