Here is the code for creating a file signature using multithreading.

It is required to write utilite with C++ for generating of file signature.

Signature will be generating as follows: origin file should be divided into equal parts.
If the size of a file is not equally divided on size of a block, the last size can be less or can be added by zeros.
For each block is calculating the value of a hash function and adding to output file.

Interface:
 - Path to input file;
 - Path to output file;
 - The size of a block, by default 1 MB

The file should be optimized with multithreading
 - The correct processing of all errors based on exceptions should be provided.
 - For resources should be used smart ptrs;
 - It is forbidden to use additional libraries;
 - The utilite should correctly work for Linux, x86 and x64

File is divided into equal parts. 
The chunk is read by the Thread produceData;
The hash function is calculating in Thread-pull tasks;
Tht results of calculating are collecting in the Thread resumeData.

Possible improvments:
To consider other hash funciton;
To consider non-blocking thread-safe queue;
To simplify code;
To test with different implementaions;

To build without tests:
```
git clone -b master --recursive git@github.com:SafinaM/file_signature.git 
cd file_sinature
mkdir build
cd build
cmake ../scc
cmake --build . --config Release -- -j 4
./file_signature <input.file> <output.file> <chunkSize in MB>
```

To build with tests:
```
git clone -b master --recursive git@github.com:SafinaM/file_signature.git
cd file_sinature
mkdir build
cd build
cmake ../src/ -DWITH_TESTS=ON
cmake --build . --config Release --target install -- -j 4
cd install/bin
./file_signature randomFile out.txt 1
./test # randomFile is provided in installation directory
```

Debug outpue will be switched on:
```
id = 0, hash = 4816188159313654
id = 1, hash = 7375261180985723587
id = 2, hash = 9338698269637873810
id = 3, hash = 1042236991086119991
id = 4, hash = 11189937755857998078
id = 5, hash = 15128584199098814793
id = 6, hash = 13547153732793486867
id = 7, hash = 13436507874639234342
id = 8, hash = 8088573131653821677
...

```

Test output
```
FileReader test...
FileReader::FileReader: file test.bin not found
Exception: FileReader::FileReader: Fstream was not opened!
FileWriter test...
ThreadPool test...
ChunkProcessor test...

CnunkSize = 1048576
id = 0, hash = 15857475300751614869
id = 1, hash = 14176532605543503582
id = 2, hash = 8962366113947489909

CnunkSize = 2097152
id = 0, hash = 8064597328693366090
id = 1, hash = 8962366113947489909

CnunkSize = 3145728
id = 0, hash = 4392418993377932163

Success!
```

Benchmarks with hyperfine, 1 GB file, 1 launch with chunkSize = 1 MB, 
```
hyperfine -r 30 --export-markdown md.md './file_signature file.bin output.txt 100'
```

| Command | Mean [s] | Min [s] | Max [s] | Relative |
|:---|---:|---:|---:|---:|
| `./file_signature file.bin output.txt 1` | 2.487 ± 0.121 | 2.249 | 2.714 | 1.00 |
| `./file_signature_one_thread file.bin output.txt 1` | 6.099 ± 0.368 | 5.745 | 7.139 | 1.00 |

the second with chunkSize = 100 MB

| Command | Mean [s] | Min [s] | Max [s] | Relative |
|:---|---:|---:|---:|---:|
| `./file_signature file.bin output.txt 100` | 3.399 ± 0.141 | 3.085 | 3.658 | 1.00 |
| `./file_signature file.bin output.txt 100` | 6.992 ± 0.248 | 6.427 | 7.331 | 1.00 |

For chunkSize = 1MB, the multi-threaded implementation was 2.5 times faster than the single-threaded one.
For chunkSize = 100MB multi-threaded implementation was 2.1 times faster than single-threaded.
