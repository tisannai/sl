PROJECT_CEEDLING_ROOT = "vendor/ceedling"
load "#{PROJECT_CEEDLING_ROOT}/lib/ceedling.rb"

Ceedling.load_project

task :default => %w[ test:all release publish ]

require 'fileutils'
task :publish do
    Rake::Task["release"].invoke
    FileUtils.cp( "build/release/libsl.so.0.0.1", "#{ENV['HOME']}/usr/lib" )
    FileUtils.cp( "src/sl.h", "#{ENV['HOME']}/usr/include" )
end
