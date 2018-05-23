#!/usr/bin/ruby
require 'xcodeproj'
project_path = 'laidoff.xcodeproj'
project = Xcodeproj::Project.open(project_path)
puts 'List of targets'
project.targets.each do |target|
    puts target.name
end
target = project.targets.first


new_file_name = ARGV[0] #'../src/render_ttl.c'
group_name = 'laidoff/src'
new_file = project.new_file(new_file_name)

target.source_build_phase.add_file_reference(new_file, true)

files = target.source_build_phase.files.to_a.map do |pbx_build_file|
    pbx_build_file.file_ref.real_path.to_s
end.select do |path|
    path.end_with?(".c")
end.select do |path|
    File.exists?(path)
end
    
files.each do |f|
    puts f
end

project.save(project_path)

