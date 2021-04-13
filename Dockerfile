FROM docker.io/debian
RUN apt-get update && apt-get upgrade -y
RUN apt-get install libcairomm-1.0-dev -y
RUN useradd samal_user
USER samal_user
COPY cmake-build-release/samal_cli/samal /opt/samal
EXPOSE 8080
WORKDIR /opt
CMD /opt/samal