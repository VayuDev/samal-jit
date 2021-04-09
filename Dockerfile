FROM docker.io/debian
RUN apt update && apt upgrade -y
RUN useradd samal_user
USER samal_user
COPY cmake-build-release/samal_cli/samal /opt/samal
EXPOSE 8080
WORKDIR /opt
CMD /opt/samal
