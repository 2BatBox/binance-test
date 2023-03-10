# binace-test

#What are the dependencies?
```
libcurl4-gnutls-dev >= 7.68.0
libssl-dev >= 1.1.1
libjsoncpp-dev >= 1.7.4
libwebsockets-dev >= 15
```

#How to build the sample?

```
sudo apt-get install cmake make gcc g++ libcurl4-gnutls-dev libssl-dev libjsoncpp-dev libwebsockets-dev
```


```
git clone https://github.com/2SilentJay/binance-test.git
cd binance-test
mkdir build
cd build
cmake ../
make
./bintest [options]
```
