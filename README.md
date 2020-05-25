# Project_OperativeSystemsLab-2019
Simple client-server architecture that represents a "Object-Store" like system 

# Files

- server.c => It contains the server which represents the Object Store in which you can register an account, store and retrieve
              data
- arch.h, arch.c, utils.h => This are the files that build the architecture of the server, like the data structure in which the 
                             server store the data and the functions that it uses to connect with the client
- betaclient.c, client.c, connection.c, connection.h => This files contain the structure of the client itself and the mechanisms 
                                                        with the client can interact with the server
