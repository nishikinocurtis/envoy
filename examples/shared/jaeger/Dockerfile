FROM jaegertracing/all-in-one@sha256:8433936d85dda4233352b800ceb054d6ceb54ac361923603669b3b406f7b0515
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
