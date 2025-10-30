// shop_manager.c
// Simple command-line shop manager for small shops.
// Features: add/update/delete products, record sales, stock alerts, reports, CSV export.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PRODUCTS_FILE "products.dat"
#define SALES_FILE "sales.csv"
#define MAX_PRODUCTS 1000
#define NAME_LEN 64
#define BUFFER 128

typedef struct {
    int id;
    char name[NAME_LEN];
    double price;
    int stock;
} Product;

Product products[MAX_PRODUCTS];
int product_count = 0;

void trim_newline(char *s) {
    size_t l = strlen(s);
    if (l && s[l - 1] == '\n')
        s[l - 1] = 0;
}

int load_products() {
    FILE *f = fopen(PRODUCTS_FILE, "rb");
    if (!f) return 0;
    if (fread(&product_count, sizeof(int), 1, f) != 1) {
        fclose(f);
        return 0;
    }
    if (product_count > MAX_PRODUCTS) {
        fclose(f);
        return 0;
    }
    fread(products, sizeof(Product), product_count, f);
    fclose(f);
    return 1;
}

int save_products() {
    FILE *f = fopen(PRODUCTS_FILE, "wb");
    if (!f) {
        perror("Save products");
        return 0;
    }
    fwrite(&product_count, sizeof(int), 1, f);
    fwrite(products, sizeof(Product), product_count, f);
    fclose(f);
    return 1;
}

int find_product_index_by_id(int id) {
    for (int i = 0; i < product_count; i++)
        if (products[i].id == id) return i;
    return -1;
}

int next_id() {
    int m = 0;
    for (int i = 0; i < product_count; i++)
        if (products[i].id > m) m = products[i].id;
    return m + 1;
}

void list_products(int show_low_only) {
    printf("ID  Name                             Price    Stock  Low\n");
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < product_count; i++) {
        Product *p = &products[i];
        int low = (p->stock <= 5);
        if (show_low_only && !low) continue;
        printf("%-3d %-32s %7.2f %7d   %s\n", p->id, p->name, p->price, p->stock, low ? "YES" : "");
    }
}

void add_product() {
    if (product_count >= MAX_PRODUCTS) {
        printf("Product limit reached.\n");
        return;
    }
    char buf[BUFFER];
    Product p;
    p.id = next_id();

    printf("Name: ");
    if (!fgets(buf, BUFFER, stdin)) return;
    trim_newline(buf);
    strncpy(p.name, buf, NAME_LEN - 1);
    p.name[NAME_LEN - 1] = 0;

    printf("Price: ");
    if (!fgets(buf, BUFFER, stdin)) return;
    p.price = atof(buf);

    printf("Stock: ");
    if (!fgets(buf, BUFFER, stdin)) return;
    p.stock = atoi(buf);

    products[product_count++] = p;
    save_products();
    printf("Added product ID %d.\n", p.id);
}

void update_product() {
    char buf[BUFFER];
    printf("Enter product ID to update: ");
    if (!fgets(buf, BUFFER, stdin)) return;
    int id = atoi(buf);
    int idx = find_product_index_by_id(id);
    if (idx < 0) {
        printf("Not found.\n");
        return;
    }
    Product *p = &products[idx];

    printf("Current name: %s\nNew name (leave empty to keep): ", p->name);
    if (!fgets(buf, BUFFER, stdin)) return;
    trim_newline(buf);
    if (strlen(buf)) strncpy(p->name, buf, NAME_LEN - 1);

    printf("Current price: %.2f\nNew price (leave empty to keep): ", p->price);
    if (!fgets(buf, BUFFER, stdin)) return;
    trim_newline(buf);
    if (strlen(buf)) p->price = atof(buf);

    printf("Current stock: %d\nNew stock (leave empty to keep): ", p->stock);
    if (!fgets(buf, BUFFER, stdin)) return;
    trim_newline(buf);
    if (strlen(buf)) p->stock = atoi(buf);

    save_products();
    printf("Product updated.\n");
}

void delete_product() {
    char buf[BUFFER];
    printf("Enter product ID to delete: ");
    if (!fgets(buf, BUFFER, stdin)) return;
    int id = atoi(buf);
    int idx = find_product_index_by_id(id);
    if (idx < 0) {
        printf("Not found.\n");
        return;
    }
    for (int i = idx; i < product_count - 1; i++)
        products[i] = products[i + 1];
    product_count--;
    save_products();
    printf("Deleted.\n");
}

void get_date_str(time_t t, char *out, size_t n) {
    struct tm tm = *localtime(&t);
    strftime(out, n, "%Y-%m-%d", &tm);
}

time_t parse_date(const char *s) {
    struct tm tm = {0};
#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE)
    if (strptime(s, "%Y-%m-%d", &tm) == NULL) return (time_t)0;
#else
    // Fallback if strptime not available (Windows)
    sscanf(s, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
#endif
    return mktime(&tm);
}

void append_sale_csv(int product_id, const char *product_name, int qty, double price) {
    FILE *f = fopen(SALES_FILE, "a");
    if (!f) {
        perror("Append sale");
        return;
    }
    char datestr[32];
    get_date_str(time(NULL), datestr, sizeof(datestr));
    double total = price * qty;
    fprintf(f, "%s,%d,\"%s\",%d,%.2f,%.2f\n", datestr, product_id, product_name, qty, price, total);
    fclose(f);
}

void record_sale() {
    char buf[BUFFER];
    printf("Enter product ID: ");
    if (!fgets(buf, BUFFER, stdin)) return;
    int id = atoi(buf);
    int idx = find_product_index_by_id(id);
    if (idx < 0) {
        printf("Not found.\n");
        return;
    }
    Product *p = &products[idx];
    printf("Enter quantity sold: ");
    if (!fgets(buf, BUFFER, stdin)) return;
    int qty = atoi(buf);
    if (qty <= 0) {
        printf("Invalid qty.\n");
        return;
    }
    if (qty > p->stock) {
        printf("Insufficient stock (%d available).\n", p->stock);
        return;
    }
    p->stock -= qty;
    append_sale_csv(p->id, p->name, qty, p->price);
    save_products();
    printf("Sale recorded. Remaining stock: %d\n", p->stock);
}

void export_products_csv() {
    FILE *f = fopen("products_export.csv", "w");
    if (!f) {
        perror("Export");
        return;
    }
    fprintf(f, "id,name,price,stock\n");
    for (int i = 0; i < product_count; i++) {
        fprintf(f, "%d,\"%s\",%.2f,%d\n", products[i].id, products[i].name, products[i].price, products[i].stock);
    }
    fclose(f);
    printf("Exported to products_export.csv\n");
}

void generate_report() {
    char buf[BUFFER];
    printf("Report range in days (e.g., 1 for today, 7 for last 7 days): ");
    if (!fgets(buf, BUFFER, stdin)) return;
    int days = atoi(buf);
    if (days <= 0) {
        printf("Invalid days.\n");
        return;
    }
    FILE *f = fopen(SALES_FILE, "r");
    if (!f) {
        printf("No sales recorded yet.\n");
        return;
    }
    time_t now = time(NULL);
    time_t cutoff = now - (time_t)(days - 1) * 24 * 3600; // include today
    char line[512];
    double total_revenue = 0;
    int total_qty = 0;

    printf("Date       ID Name                           Qty  Price   Total\n");
    printf("----------------------------------------------------------------\n");

    // Skip header
    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f)) {
        char date[32], name[128];
        int id, qty;
        double price, total;
        if (sscanf(line, "%31[^,],%d,\"%127[^\"]\",%d,%lf,%lf",
                   date, &id, name, &qty, &price, &total) == 6) {
            time_t sale_time = parse_date(date);
            if (sale_time == 0) continue;

            struct tm tm_cut = *localtime(&cutoff);
            tm_cut.tm_hour = 0; tm_cut.tm_min = 0; tm_cut.tm_sec = 0;
            time_t cutoff_start = mktime(&tm_cut);

            if (sale_time >= cutoff_start) {
                printf("%-10s %-3d %-30s %4d %7.2f %8.2f\n", date, id, name, qty, price, total);
                total_qty += qty;
                total_revenue += total;
            }
        }
    }
    fclose(f);
    printf("----------------------------------------------------------------\n");
    printf("Total items sold: %d\nTotal revenue: %.2f\n", total_qty, total_revenue);
}

void show_menu() {
    printf("\nShop Manager\n");
    printf("1) List all products\n");
    printf("2) Add product\n");
    printf("3) Update product\n");
    printf("4) Delete product\n");
    printf("5) Record sale\n");
    printf("6) List low stock products\n");
    printf("7) Generate sales report\n");
    printf("8) Export products to CSV\n");
    printf("9) Exit\n");
    printf("Choose: ");
}

int main() {
    load_products();

    // ensure sales file has header if not exists
    FILE *sf = fopen(SALES_FILE, "r");
    if (!sf) {
        sf = fopen(SALES_FILE, "w");
        if (sf) {
            fprintf(sf, "date,product_id,product_name,qty,price,total\n");
            fclose(sf);
        }
    } else fclose(sf);

    char buf[BUFFER];
    while (1) {
        show_menu();
        if (!fgets(buf, BUFFER, stdin)) break;
        int choice = atoi(buf);
        switch (choice) {
            case 1: list_products(0); break;
            case 2: add_product(); break;
            case 3: update_product(); break;
            case 4: delete_product(); break;
            case 5: record_sale(); break;
            case 6: list_products(1); break;
            case 7: generate_report(); break;
            case 8: export_products_csv(); break;
            case 9: printf("Bye.\n"); exit(0);
            default: printf("Invalid.\n"); break;
        }
    }
    return 0;
}
