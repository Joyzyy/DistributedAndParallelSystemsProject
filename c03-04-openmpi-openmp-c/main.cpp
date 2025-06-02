#include <mpi.h>
#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <omp.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <openssl/evp.h>
#include <openssl/aes.h>

class AESCipher {
private:
    unsigned char key[16];
    unsigned char iv[16];

public:
    AESCipher(const std::string& key_str) {
        if (key_str.size() != 16) {
            throw std::invalid_argument("Key must be 16 bytes for AES-128");
        }
        std::copy(key_str.begin(), key_str.end(), key);
        // Initialize IV to zero or any other value
        std::fill(iv, iv + 16, 0);
    }

    int encrypt_aes_cbc(const unsigned char* plaintext, int plaintext_len,
        unsigned char* ciphertext 
    ) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -1;
        
        int len, ciphertext_len;

        if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        
        if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        ciphertext_len = len;
        
        if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        ciphertext_len += len;

        EVP_CIPHER_CTX_free(ctx);
        return ciphertext_len;
    }

    int encrypt_aes_ecb(const unsigned char* plaintext, int plaintext_len,
                        unsigned char* ciphertext) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -1;
        
        int len;
        int ciphertext_len;

        if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        
        if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        ciphertext_len = len;
        
        if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        ciphertext_len += len;

        EVP_CIPHER_CTX_free(ctx);
        return ciphertext_len;
    }

    int decrypt_aes_cbc(const unsigned char* ciphertext, int ciphertext_len,
                        unsigned char* plaintext) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -1;
        
        int len;
        int plaintext_len;

        if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        
        if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        plaintext_len = len;
        
        if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        plaintext_len += len;

        EVP_CIPHER_CTX_free(ctx);
        return plaintext_len;
    }

    int decrypt_aes_ecb(const unsigned char* ciphertext, int ciphertext_len,
                        unsigned char* plaintext) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -1;
        
        int len;
        int plaintext_len;

        if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        
        if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        plaintext_len = len;
        
        if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        plaintext_len += len;

        EVP_CIPHER_CTX_free(ctx);
        return plaintext_len;
    }
};

int main(int argc, char** argv) {
    /*
        argv[1] = filename
        argv[2] = encrypt/decrypt
        argv[3] = aes-128-cbc/aes-128-ecb
        argv[4] = key
    */
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << "mpirun -np <n> --host <hosts> executable_mpi <filename> <encrypt/decrypt> <aes-128-cbc/aes-128-ecb> <key>" << std::endl;
        return -1;
    }

    std::string filename = argv[1];
    std::string operation = argv[2];
    std::string mode = argv[3];
    std::string key = argv[4];
    std::string filename_without_extenstion = filename.substr(0, filename.find_last_of("."));

    if (operation != "encrypt" && operation != "decrypt") {
        std::cerr << "Invalid operation. Use 'encrypt' or 'decrypt'." << std::endl;
        return -1;
    }

    if (mode != "aes-128-cbc" && mode != "aes-128-ecb") {
        std::cerr << "Invalid mode. Use 'aes-128-cbc' or 'aes-128-ecb'." << std::endl;
        return -1;
    }

    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);

    std::cout << "Hello from process " << world_rank << " of " << world_size 
              << " running on container: " << hostname << std::endl;

    std::vector<char> buffer;
    size_t total_size = 0;

    // only rank 0(c03) reads the file
    if (world_rank == 0) {
        std::ifstream input_file(filename, std::ios::binary);
        if (!input_file) {
            std::cerr << "Error opening input file." << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        input_file.seekg(0, std::ios::end);
        total_size = input_file.tellg();
        input_file.seekg(0, std::ios::beg);
        
        buffer.resize(total_size);
        input_file.read(buffer.data(), total_size);
        input_file.close();

        std::cout << "Rank 0: Read file of size " << total_size << " bytes." << std::endl;
    }

    MPI_Bcast(&total_size, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

    size_t chunk_size = total_size / world_size;
    size_t remainder = total_size % world_size;
    
    size_t my_chunk_size = chunk_size;
    if (world_rank == world_size - 1) {
        my_chunk_size += remainder;
    }

    std::vector<char> my_chunk(my_chunk_size);
    if (world_rank == 0) {
        size_t offset = 0;
        for (size_t i = 0; i < world_size; i++) {
            size_t send_size = chunk_size;
            if (i == world_size - 1) {
                send_size += remainder;
            }

            if (i == 0) {
                std::copy(buffer.begin(), buffer.begin() + send_size, my_chunk.begin());
            } else {
                MPI_Send(buffer.data() + offset, send_size, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            }
            offset += send_size;
        }
    } else {
        MPI_Recv(my_chunk.data(), my_chunk_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    std::cout << "Process " << world_rank << " recieved chunk of size " 
              << my_chunk_size << " bytes." << std::endl;

    try {
        AESCipher cipher(key);
        double start_time = MPI_Wtime();
        
        if (operation == "encrypt") {
            if (mode == "aes-128-cbc") {
                std::vector<unsigned char> encrypted_chunk(my_chunk_size + AES_BLOCK_SIZE);
        
                int encrypted_len = cipher.encrypt_aes_cbc(reinterpret_cast<const unsigned char*>(my_chunk.data()),
                                                            my_chunk_size, encrypted_chunk.data()
                                                        );
                if (encrypted_len < 0) {
                    throw std::runtime_error("Encryption failed in AES-CBC mode.");
                }

                if (world_rank == 0) {
                    std::vector<unsigned char> all_encrypted_data;
                    all_encrypted_data.insert(all_encrypted_data.end(), encrypted_chunk.begin(), encrypted_chunk.begin() + encrypted_len);

                    for (int i = 1; i < world_size; i++) {
                        int recv_encrypted_len;
                        MPI_Recv(&recv_encrypted_len, 1, MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        std::vector<unsigned char> recv_encrypted(recv_encrypted_len);
                        MPI_Recv(recv_encrypted.data(), recv_encrypted_len, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        
                        std::cout << "Rank 0 received encrypted data from rank " << i 
                                << " of size " << recv_encrypted_len << " bytes." << std::endl;
                        all_encrypted_data.insert(all_encrypted_data.end(), recv_encrypted.begin(), recv_encrypted.end());
                    }

                    std::string output_file_name = filename_without_extenstion + "_output.bin";
                    std::ofstream output_file(output_file_name, std::ios::binary);
                    output_file.write(reinterpret_cast<const char*>(all_encrypted_data.data()), all_encrypted_data.size());
                    output_file.close();

                    std::cout << "Rank 0: Wrote encrypted data to " << output_file_name 
                            << " of size " << all_encrypted_data.size() << " bytes." << std::endl;
                } else {
                    MPI_Send(&encrypted_len, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
                    MPI_Send(encrypted_chunk.data(), encrypted_len, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
                }
            } else if (mode == "aes-128-ecb") {
                std::vector<unsigned char> encrypted_chunk(my_chunk_size + AES_BLOCK_SIZE);

                int num_blocks = my_chunk_size / AES_BLOCK_SIZE;
                int remaining_bytes = my_chunk_size % AES_BLOCK_SIZE;

                int encrypted_len = 0;

                #pragma omp parallel for reduction(+:encrypted_len)
                for (int block = 0; block < num_blocks; block++) {
                    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
                    if (ctx) {
                        int len;
                        if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, reinterpret_cast<const unsigned char*>(key.c_str()), NULL) == 1) {
                            EVP_CIPHER_CTX_set_padding(ctx, 0);

                            const unsigned char* input = reinterpret_cast<const unsigned char*>(my_chunk.data() + block * AES_BLOCK_SIZE);
                            unsigned char* output = encrypted_chunk.data() + block * AES_BLOCK_SIZE;

                            if (EVP_EncryptUpdate(ctx, output, &len, input, AES_BLOCK_SIZE) == 1) {
                                #pragma omp atomic
                                encrypted_len += len;
                            }
                        }
                        EVP_CIPHER_CTX_free(ctx);
                    }
                }

                if (remaining_bytes > 0) {
                    int final_len = cipher.encrypt_aes_ecb(
                        reinterpret_cast<const unsigned char*>(my_chunk.data() + num_blocks * AES_BLOCK_SIZE), remaining_bytes, encrypted_chunk.data() + num_blocks * AES_BLOCK_SIZE);
                    if (final_len < 0) {
                        throw std::runtime_error("Encryption failed in AES-ECB mode.");
                    }
                } else {
                    encrypted_len += num_blocks * AES_BLOCK_SIZE;
                }

                if (encrypted_len < 0) {
                    throw std::runtime_error("Encryption failed in AES-ECB mode.");
                }

                if (world_rank == 0) {
                    std::vector<unsigned char> all_encrypted_data;
                    all_encrypted_data.insert(all_encrypted_data.end(), encrypted_chunk.begin(), encrypted_chunk.begin() + encrypted_len);

                    for (int i = 1; i < world_size; i++) {
                        int recv_encrypted_len;
                        MPI_Recv(&recv_encrypted_len, 1, MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        std::vector<unsigned char> recv_encrypted(recv_encrypted_len);
                        MPI_Recv(recv_encrypted.data(), recv_encrypted_len, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        
                        std::cout << "Rank 0 received encrypted data from rank " << i 
                                << " of size " << recv_encrypted_len << " bytes." << std::endl;
                        all_encrypted_data.insert(all_encrypted_data.end(), recv_encrypted.begin(), recv_encrypted.end());
                    }

                    std::string output_file_name = filename_without_extenstion + "_output.bin";
                    std::ofstream output_file(output_file_name, std::ios::binary);
                    output_file.write(reinterpret_cast<const char*>(all_encrypted_data.data()), all_encrypted_data.size());
                    output_file.close();

                    std::cout << "Rank 0: Wrote encrypted data to " << output_file_name 
                            << " of size " << all_encrypted_data.size() << " bytes." << std::endl;
                } else {
                    MPI_Send(&encrypted_len, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
                    MPI_Send(encrypted_chunk.data(), encrypted_len, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
                }
            }
        } else if (operation == "decrypt") {
            if (mode == "aes-128-cbc") {
                std::cout << "Process " << world_rank << " starting decryption." << std::endl;
    
                std::vector<unsigned char> decrypted_chunk(my_chunk_size + AES_BLOCK_SIZE);
                int decrypted_len = cipher.decrypt_aes_cbc(reinterpret_cast<const unsigned char*>(my_chunk.data()),
                                                            my_chunk_size, decrypted_chunk.data()
                                                        );
                if (decrypted_len < 0) {
                    throw std::runtime_error("Decryption failed in AES-CBC mode.");
                }
    
                if (world_rank == 0) {
                    std::vector<unsigned char> all_decrypted_data;
                    all_decrypted_data.insert(all_decrypted_data.end(), decrypted_chunk.begin(), decrypted_chunk.begin() + decrypted_len);
    
                    for (int i = 1; i < world_size; i++) {
                        int recv_decrypted_len;
                        MPI_Recv(&recv_decrypted_len, 1, MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
                        std::vector<unsigned char> recv_decrypted(recv_decrypted_len);
                        MPI_Recv(recv_decrypted.data(), recv_decrypted_len, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        
                        std::cout << "Rank 0 received decrypted data from rank " << i 
                                << " of size " << recv_decrypted_len << " bytes." << std::endl;
                        all_decrypted_data.insert(all_decrypted_data.end(), recv_decrypted.begin(), recv_decrypted.end());
                    }
    
                    std::string output_file_name = filename_without_extenstion + "_outputdecrypted.bmp";
                    std::ofstream output_file(output_file_name, std::ios::binary);
                    output_file.write(reinterpret_cast<const char*>(all_decrypted_data.data()), all_decrypted_data.size());
                    output_file.close();
    
                    std::cout << "Rank 0: Wrote decrypted data to " << output_file_name 
                            << " of size " << all_decrypted_data.size() << " bytes." << std::endl;
                } else {
                    MPI_Send(&decrypted_len, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
                    MPI_Send(decrypted_chunk.data(), decrypted_len, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
                }
            } else {
                std::cout << "Process " << world_rank << " starting decryption." << std::endl;

                std::vector<unsigned char> decrypted_chunk(my_chunk_size + AES_BLOCK_SIZE);
                
                int num_blocks = my_chunk_size / AES_BLOCK_SIZE;
                int remaining_bytes = my_chunk_size % AES_BLOCK_SIZE;
                
                int decrypted_len = 0;
                
                #pragma omp parallel for reduction(+:decrypted_len)
                for (int block = 0; block < num_blocks; block++) {
                    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
                    if (ctx) {
                        int len;
                        if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, 
                                             reinterpret_cast<const unsigned char*>(key.c_str()), NULL) == 1) {
                            EVP_CIPHER_CTX_set_padding(ctx, 0);
                            
                            const unsigned char* input = reinterpret_cast<const unsigned char*>(my_chunk.data()) 
                                                        + block * AES_BLOCK_SIZE;
                            unsigned char* output = decrypted_chunk.data() + block * AES_BLOCK_SIZE;
                            
                            if (EVP_DecryptUpdate(ctx, output, &len, input, AES_BLOCK_SIZE) == 1) {
                                #pragma omp atomic
                                decrypted_len += len;
                            }
                        }
                        EVP_CIPHER_CTX_free(ctx);
                    }
                }
                
                if (remaining_bytes > 0) {
                    int final_len = cipher.decrypt_aes_ecb(
                        reinterpret_cast<const unsigned char*>(my_chunk.data()) + num_blocks * AES_BLOCK_SIZE,
                        remaining_bytes, 
                        decrypted_chunk.data() + num_blocks * AES_BLOCK_SIZE
                    );
                    if (final_len > 0) {
                        decrypted_len += final_len;
                    }
                } else {
                    decrypted_len = num_blocks * AES_BLOCK_SIZE;
                }

                if (decrypted_len <= 0) {
                    throw std::runtime_error("Decryption failed in AES-ECB mode.");
                }
    
                if (world_rank == 0) {
                    std::vector<unsigned char> all_decrypted_data;
                    all_decrypted_data.insert(all_decrypted_data.end(), decrypted_chunk.begin(), decrypted_chunk.begin() + decrypted_len);
    
                    for (int i = 1; i < world_size; i++) {
                        int recv_decrypted_len;
                        MPI_Recv(&recv_decrypted_len, 1, MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
                        std::vector<unsigned char> recv_decrypted(recv_decrypted_len);
                        MPI_Recv(recv_decrypted.data(), recv_decrypted_len, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        
                        std::cout << "Rank 0 received decrypted data from rank " << i 
                                << " of size " << recv_decrypted_len << " bytes." << std::endl;
                        all_decrypted_data.insert(all_decrypted_data.end(), recv_decrypted.begin(), recv_decrypted.end());
                    }
    
                    std::string output_file_name = filename_without_extenstion + "_outputdecrypted.bmp";
                    std::ofstream output_file(output_file_name, std::ios::binary);
                    output_file.write(reinterpret_cast<const char*>(all_decrypted_data.data()), all_decrypted_data.size());
                    output_file.close();
    
                    std::cout << "Rank 0: Wrote decrypted data to " << output_file_name 
                            << " of size " << all_decrypted_data.size() << " bytes." << std::endl;
                } else {
                    MPI_Send(&decrypted_len, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
                    MPI_Send(decrypted_chunk.data(), decrypted_len, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
                }
            }
        } else {
            throw std::invalid_argument("Invalid operation. Use 'encrypt' or 'decrypt'.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Finalize();
    return 0;
}