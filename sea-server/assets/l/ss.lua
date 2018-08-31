print('loading ss.lua...')

function reload()
    package.loaded['assets/l/admin'] = nil
    require('assets/l/admin')
    inspect = require('assets/l/inspect')
end

reload()
