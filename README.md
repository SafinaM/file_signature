Here presented code to create of file's signature using multi-threading

It is required to write utilite with C++ for generating of file signature.

Signature will be generating as follows: origin file should be divided into equal parts.
If the size of a file is not equally divided on size of a block, the last size can be less or can be added by zeros.
For each block is calculating the value of a hash function and adding to output file.

Interface:
Path to input file;
Path to output file;
The size of a block, by default 1 MB

The file should be optimized with multithreading
The correct processing of all errors based on exceptions should be provided.
For resources should be used smart ptrs;
It is forbidden to use additional libraries;
The utilite should correctly work for Linux, x86 and x64

File is divided into equal parts. 
The chunk is read by the Thread produceData;
The hash function is calculating in thread pull task;
Tht result of calculating is written in yet thread resumeData;
For collecting result we use additional thread checkResult.
