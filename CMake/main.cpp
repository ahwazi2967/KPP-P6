#include "interpolation.h"
#include <iostream>
#include <vector>
#include "serial.h"
#include <unistd.h>

int main() {
    double a1, a2, a3;
    std::cout << "Masukkan 3 sudut: ";
    std::cin >> a1 >> a2 >> a3;

    std::vector<double> angles = {a1, a2, a3};
    std::vector<int> digital_units = interpolate_list(angles);
    
    std::string data_str;
    for (size_t i = 0; i < digital_units.size(); i++) {
        data_str += std::to_string(digital_units[i]);
        if (i + 1 < digital_units.size()) data_str += ' ';
    }
    data_str += '\n';

    try {
        std::string port_name = find_serial_port();
        std::cout << "Pakai port: " << port_name << '\n';
        int fd = open_serial(port_name);
        
        // Poin 1: TX (Kirim ke STM32)
        send_serial(fd, data_str);
        std::cout << "[PC TX] Data terkirim: " << data_str;
        
        usleep(200000); // 200ms delay for STM32 to process & reply
        
        // Poin 1: RX (Terima balasan dari STM32)
        std::string balas = receive_serial(fd);
        std::cout << "[PC RX] Balasan dari STM32: " << balas;
        
        close(fd);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
