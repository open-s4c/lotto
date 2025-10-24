
## tools:

gcc, clang, clang-format, clang-tidy, gcovr, lit and cmake-format


## maximas

- envvars are allowed for hacks and only hacks
- hacks are not officially supported

## docker

We advise to use docker to run several important targets.
Install it via your OS's package manager, enable its service and add current
user to the docker group.

Ubuntu/Debian:

``` shell
sudo apt update
sudo apt install -y docker.io
sudo systemctl enable docker
sudo systemctl start docker
sudo usermod -aG docker $USER # Add yourself to a docker group
newgrp docker # Apply changes without relogin
```

ArchLinux:

``` shell
sudo pacman -Syu
sudo pacman -S --noconfirm docker
sudo systemctl enable docker
sudo systemctl start docker
sudo usermod -aG docker $USER
newgrp docker
```

Create an access token in our gitlab capable to do the following operations:

```
api, read_api, read_user, read_repository, read_registry
```

## patch polishing

Docker must be enabled prior running cmake!
Save all files before running of any below commands!

Apply source code formatting:

``` shell
make clang-format-apply
```

Apply cmake formatting:

``` shell
make cmake-format-apply
```

Update documentation:

``` shell
make api-markdown
```
