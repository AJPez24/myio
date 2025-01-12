/*
 *myio_test.c
*/

#include "myio.c"

int main() {
    //testing intermingled reads and writes

    //first read test
    char *test_output = malloc(200);
    struct myFILE *test_file = myopen("intermingle_test.txt", "r+");
    int total_read = 0;
    printf("THIS IS WHAT WE'RE READING\n\n");
    total_read = myread(test_output, 200, test_file);
    for(int i = 0; i < 200 ; i++) {
        printf("%c", *(test_output + i));
    }
    printf("\ntotal Read %d\n\n", total_read);

    //first write test
    char test_input[2000];
        for (int i = 0; i < 2000; ++i){
            test_input[i] = '!';
        }

    printf("NOW WE'RE WRITING\n\n");
    mywrite(test_input, 50, test_file);
    
    //second read test
    printf("NOW WE'RE READING AGAIN\n\n");
    myread(test_output, 200, test_file);
    for(int i = 0; i < 200 ; i++) {
        printf("%c", *(test_output + i));
    }
    
    myclose(test_file);
}