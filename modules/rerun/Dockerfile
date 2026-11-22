FROM python:3.12-slim

# Must match RERUN_SDK_VERSION in cmake/Rerun.cmake.
ARG RERUN_VERSION=0.43.1

RUN pip install --no-cache-dir "rerun-sdk==${RERUN_VERSION}" \
    && rerun analytics disable

# Starts rerun and sends it the layout for RERUN_CAMERAS.
COPY --chmod=755 docker/entrypoint.sh tools/blueprint.py /opt/rerun/

EXPOSE 9090 9876
ENTRYPOINT ["/opt/rerun/entrypoint.sh"]
