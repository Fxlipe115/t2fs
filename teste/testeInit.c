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
    create2("fileTeste1.txt");
    mkdir2("/dir1/jujubs");
    create2("./dir1/jujubs/fileTesteJ.txt");
    rmdir2("/dir1/jujubs");
    delete2("./dir1/jujubs/fileTesteJ.txt");
    rmdir2("/dir1/jujubs");
    mkdir2("/jujubs2");
    rmdir2("batata/dir1");
    rmdir2("/jujubs2");
    return 0;
}
