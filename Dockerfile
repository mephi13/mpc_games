# requires fbpcf/ubuntu as base image
# need to buld it from here:
# https://github.com/facebookresearch/fbpcf/
FROM fbpcf/ubuntu:latest

RUN mkdir /aggregation
WORKDIR /aggregation

RUN mkdir demographic_metrics
RUN mkdir demographic_metrics_app
RUN mkdir sitecheck

COPY demographic_metrics_app/ ./demographic_metrics_app/
COPY demographic_metrics/ ./demographic_metrics/
COPY sitecheck/ ./sitecheck/
COPY CMakeLists.txt ./

RUN cmake . -DTHREADING=ON -DEMP_USE_RANDOM_DEVICE=ON -DCMAKE_CXX_FLAGS="-march=haswell" -DCMAKE_C_FLAGS="-march=haswell" -DCMAKE_BUILD_TYPE=Release && make -j4 demographicapp

CMD ["/bin/bash"]
#ENTRYPOINT [ "./bin/demographicapp" ]