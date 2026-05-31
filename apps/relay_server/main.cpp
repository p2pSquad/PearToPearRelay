#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const std::string listen_address = argc > 1 ? argv[1] : "0.0.0.0:50051";

    std::cout << "Pear relay server stub\n";
    std::cout << "listen: " << listen_address << "\n";
    std::cout << "real relay protocol is not implemented yet\n";

    return 0;
}
