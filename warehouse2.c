#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

void WH_nameFilter(WAREHOUSE *old_WH, char warehouse_name[], WAREHOUSE *new_WH, int * new_db_size, int old_db_size) {
    (*new_db_size) = 0;
    int j=0;
    for (int i = 0; i < old_db_size; i++) {
        if(strncmp(old_WH[i].name, warehouse_name, strlen(warehouse_name)) == 0){
            new_WH[j] = old_WH[i];
            j++;
            (*new_db_size)++;
        }
    }
}

void WH_gpsFilter(WAREHOUSE *old_WH, GPS warehouse_gps, WAREHOUSE *new_WH, int * new_db_size, int old_db_size){
    (*new_db_size) = 0;
    double tmp1, tmp2=20038; //najvacsia mozna vzdialenost dvoch bodov od seba na zemi
    int shortest_dist_index;
    for (int i = 0; i < old_db_size; i++) {
        tmp1 = distance(warehouse_gps, old_WH[i].gps);
        if(tmp1 < tmp2){
            shortest_dist_index = i;
            tmp2 = tmp1;
        }
    }
    new_WH[0] = old_WH[shortest_dist_index];
    (*new_db_size)++;
}

void remove_element(WAREHOUSE *WH, int * db_size, int position){
    for(int i=position; i<(*db_size)-1; i++)
    {
        WH[i] = WH[i + 1];
    }
    (*db_size)--;
}

bool load_items(WAREHOUSE *WH, FILE* itemFile, int *db_size, int position){
    bool error = 0;
    int i=0;
    int number;
    WH[position].items = (ITEM *) malloc(sizeof(ITEM));
    while((number = fscanf(itemFile, "%s %d", WH[position].items[i].name, &WH[position].items[i].price)) == 2){
        i++;
        WH[position].items = (ITEM *) realloc(WH[position].items, (i+1)*sizeof(ITEM));
    }
    WH[position].n = i;
    if(WH[position].n > WH[position].capacity){
        fprintf(stderr, "CAPACITY_ERROR %s.txt\n", WH[position].name);
        free(WH[position].items);
        remove_element(WH, db_size, position);
        error = 1;
    }
    else if(number != 2 && number !=0 && number != EOF){
        fprintf(stderr, "FORMAT_ERROR %s.txt\n", WH[position].name);
        free(WH[position].items);
        remove_element(WH, db_size, position);
        error = 1;
    }
    return error;
}

void swap(int* xp, int* yp){
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void swap2(char *str1, char *str2){
    char *temp = (char *)malloc((strlen(str1) + 1) * sizeof(char));
    strcpy(temp, str1);
    strcpy(str1, str2);
    strcpy(str2, temp);
    free(temp);
}

void call_swaps(WAREHOUSE arr[], int j){
    swap(&arr->items[j].price, &arr->items[j+1].price);
    swap2(arr->items[j].name, arr->items[j+1].name);
}

bool need_swap(WAREHOUSE arr[], int j, bool Aorder, bool Dorder){
    if (arr->items[j].price > arr->items[j+1].price && Aorder) return 1;
    if (arr->items[j].price < arr->items[j+1].price && Dorder) return 1;
    if (strcmp(arr->items[j].name, arr->items[j + 1].name) > 0 && Aorder==0 && Dorder==0) return 1;
    return 0;
}

void bubbleSort(WAREHOUSE arr[], int n, bool Aorder, bool Dorder){
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++)
            if (need_swap(arr, j, Aorder, Dorder)) {
                call_swaps(arr, j);
            }
    }
}

int main(int argc, char *argv[]){
    bool WHnameB=0, gpsB=0, ascendingOrder=0, descendingOrder=0;
    GPS warehouse_gps;
    char warehouse_name[MAX_NAME + 1];
    WAREHOUSE *db;
    int opt;
    char* optstring = ":w:n:e:ad";
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case ':':
                return 2;
            case 'w':
                WHnameB = 1;
                strcpy(warehouse_name, optarg);
                break;
            case 'n':
                gpsB = 1;
                warehouse_gps.lat = strtod(optarg, NULL);
                break;
            case 'e':
                warehouse_gps.lon = strtod(optarg, NULL);
                break;
            case 'a':
                ascendingOrder = 1;
                break;
            case 'd':
                descendingOrder = 1;
                break;
            default:
                return 1;
        }
    }
    int i=0, j, db_size;
    FILE* warehouseFile;
    FILE* itemFile;
    char itemName[MAX_NAME+7];
    char *extension = ".txt";

    warehouseFile = fopen(WAREHOUSE_DB_FILE, "r");
    db = (WAREHOUSE *) malloc(sizeof(WAREHOUSE));
    while( fscanf(warehouseFile, "%s %lf %lf %d", db[i].name, &db[i].gps.lat, &db[i].gps.lon, &db[i].capacity) == 4){ //scan skladov
        i++;
        db = (WAREHOUSE *) realloc(db, (i+1)*sizeof(WAREHOUSE));
    }
    fclose(warehouseFile);
    db_size = i;
    for (j = 0; j < i; j++) { //vypis skladov
        printf("%s %.3lf %.3lf %d\n", db[j].name, db[j].gps.lat, db[j].gps.lon, db[j].capacity);
    }
    if(WHnameB){ //name filter
        WH_nameFilter(db, warehouse_name, db, &db_size, i);
    }
    if(gpsB){ //gps filter
        WH_gpsFilter(db, warehouse_gps, db, &db_size, i);
    }
    db = (WAREHOUSE *) realloc(db, (db_size)*sizeof(WAREHOUSE));

    for (j = 0; j < db_size; j++) { //scan items + chybove situacie
        snprintf(itemName, sizeof(itemName), ITEMS_FOLDER PATH_SEPARATOR"%s%s", db[j].name, extension);
        itemFile = fopen(itemName, "r");
        if(itemFile == NULL){
            fprintf(stderr, "FILE_ERROR %s.txt\n", db[j].name);
            remove_element(db, &db_size, j);
            j--;
        }
        else{
            if(load_items(db, itemFile, &db_size, j)) j--;
        }
    }
    db = (WAREHOUSE *) realloc(db, (db_size)*sizeof(WAREHOUSE));
    fclose(itemFile);

    for (j = 0; j < db_size; j++) {
        bubbleSort(&db[j], db[j].n, ascendingOrder, descendingOrder);
    }
    int k=0;
    for (i = 0; i < db_size; i++) {
        printf("%s %.3lf %.3lf %d :\n", db[i].name, db[i].gps.lat, db[i].gps.lon, db[i].capacity);
        if (strncmp(db[k].name, db[i].name, strlen(db[k].name)) == 0) {
            for (j = 0; j < db[k].n; j++) {
                printf("%d. %s %d\n", j + 1, db[k].items[j].name, db[k].items[j].price);
            }
            k++;
        }
    }
    for( i = 0 ; i < db_size; i++)
        free(db[i].items);
    free(db); //free, inak povedane pamat zadarmo
    return 0;
}