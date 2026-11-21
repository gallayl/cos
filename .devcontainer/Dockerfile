FROM espressif/idf:release-v5.5

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN apt-get update -y \
    && apt-get install -y --no-install-recommends \
        ripgrep \
        fd-find \
        udev \
        curl \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://opencode.ai/install.sh | bash

RUN echo 'source /opt/esp/idf/export.sh > /dev/null 2>&1' >> /root/.bashrc

ENTRYPOINT ["/opt/esp/entrypoint.sh"]
CMD ["/bin/bash"]
