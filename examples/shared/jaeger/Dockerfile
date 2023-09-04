FROM jaegertracing/all-in-one@sha256:1f989c7055ffc933ba243025d7b21e326b4721e418506142da732d3c5bfb713b
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
