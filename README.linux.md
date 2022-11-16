# Install openvino


```
cd /opt
wget https://storage.openvinotoolkit.org/repositories/openvino/packages/2022.2/linux/l_openvino_toolkit_ubuntu20_2022.2.0.7713.af16ea1d79a_x86_64.tgz .

# uncompress here
tar zxvf l_openvino*.tgz
```

this version of openvino depends on `tbb.so.2` which is part of `tbb2020`, so download it too.

```
cd /opt
mkdir tbb2020

wget https://github.com/oneapi-src/oneTBB/releases/download/v2020.3/tbb-2020.3-lin.tgz
tar zxvf tbb-2020.3-lin.tgz -C /opt/tbb2020

export LD_LIBRARY_PATH=/opt/tbb2020/tbb/lib/intel64/gcc4.8/

# now start build
make build-lib
```
