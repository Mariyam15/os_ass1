#include <cerrno>
}


int p2c[2]; // parent -> child
int c2p[2]; // child -> parent
if (pipe(p2c) < 0) die("pipe p2c");
if (pipe(c2p) < 0) die("pipe c2p");


pid_t pid = fork();
if (pid < 0) die("fork");


if (pid == 0) {
// Child
close(p2c[1]);
close(c2p[0]);


std::unique_ptr<uint8_t[]> buf(new uint8_t[payload]);


for (uint64_t i = 0; i < count; ++i) {
if (read_full(p2c[0], buf.get(), payload) < 0) die("child read");
if (write_full(c2p[1], buf.get(), payload) < 0) die("child write");
}


close(p2c[0]);
close(c2p[1]);
return EXIT_SUCCESS;
}


// Parent
close(p2c[0]);
close(c2p[1]);


std::unique_ptr<uint8_t[]> buf(new uint8_t[payload]);
std::unique_ptr<uint8_t[]> rbuf(new uint8_t[payload]);
memset(buf.get(), 0xAB, payload);


timespec t0{}, t1{};
if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) die("clock_gettime start");


for (uint64_t i = 0; i < count; ++i) {
if (write_full(p2c[1], buf.get(), payload) < 0) die("parent write");
if (read_full(c2p[0], rbuf.get(), payload) < 0) die("parent read");
}


if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) die("clock_gettime end");


uint64_t ns = nsec_since(t0, t1);
double sec = static_cast<double>(ns) / 1e9;
double avg_rtt_ns = static_cast<double>(ns) / static_cast<double>(count);
double one_way_ns = avg_rtt_ns / 2.0;


std::cout << "messages: " << count << ", payload: " << payload << " bytes\n";
std::cout << "elapsed: " << sec << " s\n";
std::cout << "avg RTT: " << avg_rtt_ns << " ns\n";
std::cout << "est one-way: " << one_way_ns << " ns\n";


close(p2c[1]);
close(c2p[0]);


int status = 0;
if (waitpid(pid, &status, 0) < 0) die("waitpid");
if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
return EXIT_SUCCESS;
} else {
std::cerr << "child failed\n";
return EXIT_FAILURE;
}
}
