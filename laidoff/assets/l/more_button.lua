local c = lo.script_context()
return function ()
	print('[script]more_button')
	lo.puck_game_set_show_top_level_main_menu(c.puck_game, 0)
	local html_path = lo.script_full_asset_path('html', 'more-menu.html')
	print('[script]loading HTML ' .. html_path)
	lo.htmlui_set_next_html_path(c.htmlui, html_path)
	lo.puck_game_set_show_htmlui(c.puck_game, 1)
end
