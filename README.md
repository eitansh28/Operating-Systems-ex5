# Operation-Systems-ex5
In this task we implemented a stack of strings stored on a server, allowing an unlimited amount of clients to push into the stack, pull out of the stack and look at the stack. All operations are proccessSafe. Iu used 'fcntl' for that.
We also implemented our own 'malloc' and 'free'.
### How To Run
1. download this repository.
2. open a terminal in the main folder and run the following command:  

```
make
```  

3. to run the server run the following command:  

```
./server
```  
4. next open a several separate terminal and run the following command on each terminal:  

```
./client 127.0.0.1
```  
##### basic commands
for pushing to the stack:
```
PUSH <your string>
 ```
 for poping from the stack:
 ```
 POP
 ```
 To see the string at the top of the stack:
 ```
 TOP
 ```
 to disconnect the client:
 ```
 exit
 ```
