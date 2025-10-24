FROM rnd-gitlab-eu-c.huawei.com:8443/germany-research-center/dresden-lab/s4c/utils/docker/qemu

COPY requirements.txt /.
RUN pip install --no-cache-dir --prefix /usr -r /requirements.txt
