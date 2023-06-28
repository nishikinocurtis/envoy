FROM node:20.3-bullseye-slim@sha256:00873eee0d287619672ccd368f32fa191ba43837f08d8d2dd8573b1311ed5273 as node-base


FROM node-base as node-http-auth
ARG NODE_APP=.
ARG NODE_APP_NAME=http-service/server
# Add an env to save ARG
ENV NODE_APP_PATH "/app/${NODE_APP_NAME}"
COPY "$NODE_APP" /app
# Dont use exec form to interpolate correctly
CMD node $NODE_APP_PATH
