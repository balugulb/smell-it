ARG DOCKER_TAG=v5.3.2
FROM espressif/idf:${DOCKER_TAG}

# Set locale
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# Installiere udev für USB Geräte
RUN apt-get update -y && apt-get install -y udev

# Exportiere ESP-IDF Umgebungsvariablen beim Start
RUN echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc

# Standard ENTRYPOINT & CMD
ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]
CMD ["/bin/bash", "-c"]
