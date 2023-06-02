./fs_make.x disk.fs 8192
./fs_ref.x add disk.fs smallfile
# ./fs_ref.x add disk.fs myfile
make
./test_read.x disk.fs > output1

head -c 4096 smallfile > output2
# cat myfile > output2
diff output1 output2
