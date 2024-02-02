# avr-node
## Build
### Release ###
```
mkdir ./build
cd ./build
cmake ..
make
```
### Debug ###
```
mkdir ./build
cd ./build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```
## Flash
### Flash fuses (optional) ###
```
make fuse
```
### Flash firmware ###
```
make flash
```
### Read MCU info ###
```
make info
```
