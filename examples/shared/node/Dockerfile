FROM node:20.7-bullseye-slim@sha256:4b541ff7fa3d7b9372ac540c53a943ce740777f19c263d1330a5770887134a59 as node-base


FROM node-base as node-http-auth
ARG NODE_APP=.
ARG NODE_APP_NAME=http-service/server
# Add an env to save ARG
ENV NODE_APP_PATH "/app/${NODE_APP_NAME}"
COPY "$NODE_APP" /app
# Dont use exec form to interpolate correctly
CMD node $NODE_APP_PATH
