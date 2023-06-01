make
./fs_make.x disk.fs 100
./simple_writer.x disk.fs
./fs_ref.x ls disk.fs
./fs_ref.x info disk.fs