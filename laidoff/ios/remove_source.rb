#!/usr/bin/ruby
require 'xcodeproj'
project_path = 'laidoff.xcodeproj'
project = Xcodeproj::Project.open(project_path)
puts 'List of targets'
project.targets.each do |target|
    puts target.name
end
target = project.targets.first


remove_file_name = ARGV[0] #'render_ttl.c'
group_name = 'laidoff/src'
#remove_file = project.new_file(remove_file_name)

#target.source_build_phase.remove_file_reference(remove_file)
to_be_removed = []
files = target.source_build_phase.files.to_a.map do |pbx_build_file|
    if pbx_build_file.file_ref.real_path.to_s.end_with?(remove_file_name) then
        to_be_removed.push(pbx_build_file.file_ref)
    end

end
    
to_be_removed.each do |fr|
    puts fr.real_path.to_s
    target.source_build_phase.remove_file_reference(fr)
end

project.save(project_path)

