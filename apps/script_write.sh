# Get 3 block's worth of data from heart.csv
head -c 12288 heart.csv > data

# Get executables
make
# Create disk with 100 blocks
./fs_make.x disk.fs 100
# ./simple_writer.x disk.fs
./test_write.x disk.fs



./fs_ref.x ls disk.fs
./fs_ref.x info disk.fs