# Description
You would need to implement multiplayer 1A2B game server in hw2

# Requirement
[pdf](./Intro2NP_HW2.pdf)

# General
We will provide a docker image, you should ensure your code can run in the given image

We also provide `client.py`, `demo.py`, and sample testcases

Note that each testcase is run independently, so you need to cleanup server's data if you store them in db (disk)

## Timeline
Submission Deadline 11/29
Demo 12/1
Late Submission Deadline 12/20
Late Submission Demo 12/22

# Build
We will run `bash build.sh` to build your code, you are allowed to build the code by `Makefile`, `CMakeLists.txt`, or hard-coded commands into `build.sh`. We just run the `bash build.sh` and expect to see your exectuable file with the path `build/server`.


## Test your code
Sample testcases are given for you to test your code and the output format.
Run `python3 demo.py` to compute your scores with sample testcases.
**Please make sure your path to server is `./build/server`**

### Test specific testcase
Run `python3 demo.py -t <testcase name>`, e.g. `python3 demo.py -t create_room_sample`

You can find your output under `build/testcases_your/` and diff between correct output in `build/testcases_diff/`.

### Test your submission
Please upload a zip file called `HW2_StudentID.zip` e.g., `HW2_109550XXX.zip`.
#### Command to zip file 
```
.
├── src/
│   ├── server.cpp
│   ├── header.hpp
│   └── header.cpp
├── CMakeLists.txt
└── build.sh
```
`zip -r HW2_123.zip src/ CMakeLists.txt build.sh`

```
.
├── server.cpp
├── Makefile
├── build.sh
```
`zip -r HW2_123.zip server.cpp Makefile build.sh`

**Run `python3 demo.py -s HW2_123.zip` before submission!**

# Docker build & run
`docker pull jayzhan/np_hw2_arm` or `docker pull yuthomas/np_hw2_x86`

`docker run -it -w /home --rm -v ~/your_code_directory:/home/ jayzhan/np_hw2_arm` or `docker run -it -w /home --rm -v ~/your_code_directory:/home/ yuthomas/np_hw2_x86`

# Note
Submission that don’t follow the rule will get 20% punishment on the grade. You will get 0 points on this project for plagiarism. Please DONT do that.

# Ref
* [cpp reference](https://en.cppreference.com/w/)
* [py3.10](https://docs.python.org/3.10/)
* Effective Modern C++
* [tcp and udp using select](https://www.geeksforgeeks.org/tcp-and-udp-server-using-select/#:~:text=The%20Select%20function%20is%20used,or%20a%20specified%20time%20passes.)
* [concurrent-servers-part-1-introduction](https://eli.thegreenplace.net/2017/concurrent-servers-part-1-introduction/)
* [concurrent-servers-part-2-threads](https://eli.thegreenplace.net/2017/concurrent-servers-part-2-threads/)
* [concurrent-servers-part-3-event-driven](https://eli.thegreenplace.net/2017/concurrent-servers-part-3-event-driven/)
