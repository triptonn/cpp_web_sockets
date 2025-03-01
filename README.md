# Simple C++ Chat Server

A learning project implementing a basic chat server and client using C++ sockets with logging capabilities.

## Project Structure

project/
├── client/
│   └── myclient.cpp
├── server/
│   └── myserver.cpp
├── core/
│   ├── logger.hpp
│   └── logger.cpp
└── CMakeLists.txt

## Features
- TCP/IP socket communication
- Multi-client support using select()
- Basic logging system
- Clean shutdown handling
- Cross-platform compatibility (Linux/Unix)

## Building the Project
Requires:
- CMake (3.10 or higher)
- C++20 compatible compiler
- Unix-like environment

bash
mkdir build
cd build
cmake ..
make

## Running the Applications
From the build directory:

Server:
bash
./server

Client:
bash
./client

## Usage
### Server
- Starts on port 8080
- Type 'quit' to shutdown server
- Logs are written to server.log

### Client
- Connects to localhost:8080
- Type 'quit' to disconnect
- Messages are sent to server immediately

## Learning Objectives
- Socket Programming in C++
- Class Design and Implementation
- File I/O with logging
- Build System Configuration
- Resource Management

## License
Copyright [2025] <Nicolas Selig>
