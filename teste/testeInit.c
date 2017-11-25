#include "t2fs.h"

int main(){
    int handle;
    create2("dir1/file4.txt");
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
    create2("fileTeste.txt");
    return 0;
}
