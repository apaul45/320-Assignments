#include <stdlib.h>
//Make sure the type of argv is updated, and the second main function is updated 
extern int original_main(int argc, char** argv);

int main(int argc, char **argv) {
    original_main(argc, argv);
}
