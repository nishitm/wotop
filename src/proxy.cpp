#include "standard.h"
#include "utils.h"
#include "serversocket.h"
#include "logger.h"
#include "proxysocket.h"

using namespace std;

ServerSocket mainSocket;
char *remoteUrl;
int remotePort;
Mode mode = CLIENT;

// For closing the sockets safely when Ctrl+C SIGINT is received
void intHandler(int dummy) {
  logger(INFO) << "Closing socket";
  mainSocket.closeSocket();
  exit(0);
}

// To ignore SIGPIPE being carried over to the parent
// When client closes pipe
void pipeHandler(int dummy) {
  logger(INFO) << "Connection closed due to SIGPIPE";
}

void exchangeData(ProxySocket& sock) {

    vector<char> outBuffer((BUFSIZE+5)*sizeof(char));
    vector<char> inpBuffer((BUFSIZE+5)*sizeof(char));

    int failuresOut = 0;
    int failuresIn = 0;

    // This socket is HTTP for clients
    // but PLAIN for the server process
    // Server process talks to the SSH server
    // But Client process talks to the evil proxy
    ProxySocket outsock = ProxySocket(remoteUrl, remotePort,
                                      mode==CLIENT?HTTP:PLAIN);
    int a, b, c;

    if (mode == CLIENT) {
        outsock.sendHelloMessage();
    } else {
        sock.receiveHelloMessage();
    }

    do {
        // @a stores the number of bytes in the message
        // @b is passed by reference
        // It will store the position where the actual
        // message starts.
        // This is 0 if the message was in plaintext
        // but will vary in case of HTTP
        a = outsock.recvFromSocket(outBuffer, 0, b);
        if (a == -1) {
            // Connection has been broken
            failuresOut++;
        } else if (a == 0) {
            failuresOut = 0;
            logger(DEBUG) << "Got nothing from remote";
        } else {
            c = sock.sendFromSocket(outBuffer, b, a);
            if (c == -1) {
                failuresOut++;
            } else {
                failuresOut = 0;
            }
            logger(DEBUG) << "Sent " << c << "/" << a << " bytes from remote to local";
        }
        outBuffer[0] = 0;       // To allow sane logging

        // @a stores number of bytes in message
        // @b is passed by reference
        a = sock.recvFromSocket(inpBuffer, 0, b);

        if (a == -1) {
            // Connection has been broken
            failuresIn++;
        } else if (a == 0) {
            failuresIn = 0;
            logger(DEBUG) << "Got nothing from client";
            // TODO Send empty HTTP requests if outsock is HTTP
        } else {
            c = outsock.sendFromSocket(inpBuffer, b, a);
            if (c == -1) {
                failuresIn++;
            } else {
                failuresIn = 0;
            }
            logger(DEBUG) << "Sent " << c << "/" << a << " bytes from local to remote";
        }
        inpBuffer[0] = 0;    // To allow sane logging
        usleep(100000);

        if (failuresIn != 0 || failuresOut != 0) {
            logger(WARN) << "Failures (input): " << failuresIn;
            logger(WARN) << "Failures (output): " << failuresOut;
        }

    } while (failuresIn < 10 && failuresOut < 10);
}

void printBanner() {
    cout << "\n"
     "  ██╗    ██╗ ██████╗ ████████╗ ██████╗ ██████╗ \n"
     "  ██║    ██║██╔═══██╗╚══██╔══╝██╔═══██╗██╔══██╗\n"
     "  ██║ █╗ ██║██║   ██║   ██║   ██║   ██║██████╔╝\n"
     "  ██║███╗██║██║   ██║   ██║   ██║   ██║██╔═══╝ \n"
     "  ╚███╔███╔╝╚██████╔╝   ██║   ╚██████╔╝██║     \n"
     "   ╚══╝╚══╝  ╚═════╝    ╚═╝    ╚═════╝ ╚═╝     \n"
     "\n"
     "By Nishit (http://github.com/nishitm)\n"
     "=================================================\n\n";
}
    

int main(int argc, char * argv[]) {
    int portNumber, pid;

    printBanner();
    signal(SIGINT, intHandler);
    signal(SIGPIPE, pipeHandler);
    signal(SIGCHLD, SIG_IGN);

    // CLI argument parsing
    if (argc != 4 && argc != 5) {
        logger(ERROR) << "Usage format: ./wotop <local port> <remote url> <remotePort>";
        exit(0);
    }
    portNumber = atoi(argv[1]);
    remoteUrl = argv[2];
    remotePort = atoi(argv[3]);

    if (argc == 5) {
        if (strcmp(argv[4], "SERVER") == 0) {
            mode = SERVER;
            logger(INFO) << "Running as server";
        } else {
            logger(INFO) << "Running as client";
        }
    }

    // Class ServerSocket handles the connection logic
    mainSocket.listenOnPort(portNumber);

    // The main loop which receives and handles connections
    while (1) {
        // Accept connections and create a class instance
        // Forks as needed
        mainSocket.connectToSocket(exchangeData, mode);
    }
    return 0;
}
