local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S');
print('loading '..info.source)
