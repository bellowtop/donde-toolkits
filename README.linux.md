# Install ffmpeg opencv


```
sudo apt install libopencv-dev
sudo apt install ffmpeg
sudo apt install uuid-dev
```

# Install openvino

```
# SEE https://docs.openvino.ai/2023.1/openvino_docs_install_guides_installing_openvino_apt.html

wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
sudo apt-get install gnupg
cat /etc/lsb-release
echo "deb https://apt.repos.intel.com/openvino/2023 ubuntu22 main" | sudo tee /etc/apt/sources.list.d/intel-openvino-2023.list
sudo apt update
apt-cache search openvino
sudo apt install openvino
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
