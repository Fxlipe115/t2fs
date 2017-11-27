#include "t2fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
    //int handle;
    char *pathname;
    pathname = malloc(100);
    /*create2("dir1/file4.txt");
    create2("/dir1/file3.txt");
    create2("../dir1/file7.txt");
    delete2("file2.txt");
    open2("file2.txt");
    open2("dir/file3.txt");
    delete2("file40.txt");
    delete2("/dir1/file3.txt");
    handle = open2("file1.txt");
    delete2("./file1.txt");
    close2(handle);
    delete2("/file1.txt");
    create2("fileTeste1.txt");*/
    opendir2("./dir1");
    mkdir2("/dir1/jujubs");
    getcwd2(pathname, 100);
    printf("primeiro: %s\n", pathname);
    opendir2("/dir1/jujubs");
    //create2("./dir1/jujubs/fileTesteJ.txt");
    //rmdir2("/dir1/jujubs");
    //delete2("./dir1/jujubs/fileTesteJ.txt");
    //rmdir2("/dir1/jujubs");
    mkdir2("/jujubs2");
    //rmdir2("batata/dir1");
    //chdir2("chuchu");
    chdir2("dir1/jujubs");
    getcwd2(pathname, 100);
    printf("segundo: %s\n", pathname);
    opendir2("../");
    //create2("criaEmJu.txt");
    //rmdir2("/jujubs2");
    //delete2("criaEmJu.txt");
    //rmdir2("/jujubs2");
    chdir2("../../jujubs2");
    getcwd2(pathname, 100);
    printf("terceiro: %s\n", pathname);
    opendir2("../../");
    identify2(pathname,80);
    printf("identify: %s\n", pathname);
    return 0;
}
