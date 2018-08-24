print('loading ss.lua...')

function reload()
    package.loaded['assets/l/admin'] = nil
    require('assets/l/admin')
end

reload()
