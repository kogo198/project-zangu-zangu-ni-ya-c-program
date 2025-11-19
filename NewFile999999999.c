#include <stdio.h>   // header file
#include <string.h>  // string functions

struct patient
{
    char name[100], mail[100]; // strings
    float amount;              // bill amount
    int age;                   // age
} patient1;

int main()
{
	printf("\n----------details-------\n");
    // prompt user to enter name
    printf("Enter the name of the patient: ");
    fgets(patient1.name, sizeof(patient1.name), stdin);
    patient1.name[strcspn(patient1.name, "\n")] = '\0';

    // email
    printf("Enter the email of the patient: ");
    fgets(patient1.mail, sizeof(patient1.mail), stdin);
    patient1.mail[strcspn(patient1.mail, "\n")] = '\0';

    // amount
    printf("Enter patient bill: ");
    scanf("%f", &patient1.amount);

    // age
    printf("Enter the age of the patient: ");
    scanf("%d", &patient1.age); // FIXED ? added &

    // condition
    if (patient1.amount <= 10000 || patient1.age <= 60) {
        printf("danger!\n");
    } else {
        printf("free\n");
    }

    return 0;
}
