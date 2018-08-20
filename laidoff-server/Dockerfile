FROM node:8-alpine as builder

RUN apk add --no-cache gcc g++ make python
WORKDIR /app
COPY package.json /app/package.json
COPY package-lock.json /app/package-lock.json

COPY binding.gyp /app/binding.gyp
COPY src /app/src
COPY include /app/include
RUN npm install --production
RUN npm run gyp

COPY tsconfig.json /app/tsconfig.json
RUN npm install -g typescript
RUN npm run build

FROM node:8-alpine

WORKDIR /app
COPY --from=builder /app/node_modules /app/node_modules
COPY --from=builder /app/build /app/build
COPY --from=builder /app/.build /app/.build
COPY . /app

RUN npm run init

EXPOSE 3000 3003/udp
CMD ["npm", "run", "start"]

