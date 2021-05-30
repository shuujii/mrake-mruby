MRuby::Gem::Specification.new("mruby-require") do |spec|
  spec.license = "MIT"
  spec.author  = "KOBAYASHI Shuji"
  spec.summary = "require/require_ralative/load/autoload methods"
  spec.build.defines << "MRB_USE_REQUIRE"
end
