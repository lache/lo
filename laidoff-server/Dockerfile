FROM node:8-alpine as builder

RUN apk add --no-cache gcc g++ make python

WORKDIR /app
COPY package.json /app/package.json
COPY package-lock.json /app/package-lock.json
RUN npm install --production

FROM node:8-alpine

WORKDIR /app
COPY --from=builder /app/node_modules /app/node_modules
COPY . /app

RUN npm run init

EXPOSE 3000 3003/udp
CMD ["npm", "run", "start"]

