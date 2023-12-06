FROM jaegertracing/all-in-one@sha256:111f2b3a7c034b3cb0616ff44ccde84f0f78d18f1c03c2a248005269c9ef9c35
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
