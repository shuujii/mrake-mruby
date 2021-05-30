FIXTURE_DIR = "#{__FILE__.split('/')[0..-2].join('/')}/fixtures"
$loaded_paths = []

def mruby?
  RUBY_ENGINE == "mruby"
end

def load_paths(paths, &block)
  org_paths = $LOAD_PATH.dup
  $LOAD_PATH.replace(paths)
  begin
    block.call
  ensure
    $LOAD_PATH.replace(org_paths)
  end
end

# Workaround for errors in CRuby accessing $LOAD_PATH that seem unnecessary
def load_paths!(paths, &block)
  paths = paths.compact unless mruby?
  load_paths(paths, &block)
end

def chdir(dir, &block)
  org_dir = MRubyTest.pwd
  MRubyTest.chdir(dir)
  begin
    block.call(dir)
  ensure
    MRubyTest.chdir(org_dir)
  end
end

def chhome(home, &block)
  org_home = MRubyTest.getenv("HOME")
  home = MRubyTest.setenv("HOME", home)
  begin
    block.call(home)
  ensure
    MRubyTest.setenv("HOME", org_home)
  end
end

def assert_load_error(meth, path)
  assert "load error" do
    e = assert_raise(LoadError, "cannot load such file -- #{path}") do
      __send__ meth, path
    end
    assert_equal path, e.path
  end
end

assert "Kernel#load" do
  assert "absolute path" do
    $loaded_paths.clear
    load_paths([nil]) do  # should not read $LOAD_PATH
      path = "#{FIXTURE_DIR}/def_class.rb"
      assert_true load(path)
      assert_equal 1, TestRequireInLoadedFile.m
      assert_equal [path], $loaded_paths
    end
  end

  assert "starts with '~'" do
    $loaded_paths.clear
    load_paths([nil]) do  # should not read $LOAD_PATH
      chhome(FIXTURE_DIR) do |home|
        assert_true load("~/def_lv")
        assert_equal ["#{home}/def_lv"], $loaded_paths
      end
    end
  end

  assert "explicit relative path should use current dir as base dir" do
    $loaded_paths.clear
    load_paths([nil]) do  # should not read $LOAD_PATH
      chdir(FIXTURE_DIR) do |dir|
        assert_true load("./def_lv")
        assert_equal ["#{dir}/def_lv"], $loaded_paths
      end
    end
  end

  assert "implicit relative path should use $LOAD_PATH" do
    $loaded_paths.clear
    load_paths!(["/_non_existent_", "#{FIXTURE_DIR}/dir", FIXTURE_DIR, nil]) do
      file = "def_class.rb"
      assert_true load(file)
      assert_equal 2, TestRequireInLoadedFile.m
      assert_equal ["#{FIXTURE_DIR}/dir/#{file}"], $loaded_paths
    end
  end

  assert "$LOAD_PATH includes current directory implicitly" do
    $loaded_paths.clear
    load_paths([]) do
      chdir(FIXTURE_DIR) do
        file = "def_class.rb"
        assert_true load(file)
        assert_equal 1, TestRequireInLoadedFile.m

        # Only in this case `__FILE__` is not an absolute path
        assert_equal [file], $loaded_paths
      end
    end
  end

  assert "should expand '~' in $LOAD_PATH" do
    $loaded_paths.clear
    load_paths(["~"]) do
      chhome(FIXTURE_DIR) do |home|
        assert_true load("def_lv")
        assert_equal ["#{home}/def_lv"], $loaded_paths
      end
    end
  end

  assert "multiple load" do
    $loaded_paths.clear
    path = "#{FIXTURE_DIR}/def_lv"
    assert_true load(path)
    assert_equal [path], $loaded_paths
    assert_true load(path)
    assert_equal [path, path], $loaded_paths
  end

  assert "should not complete extension" do
    assert_load_error :load, "#{FIXTURE_DIR}/def_class"
  end

  assert "should not share local variables (#4931)" do
    load "#{FIXTURE_DIR}/def_lv"
    assert_raise(NameError){lvar_in_loaded_file}
  end

  assert "load error" do
    assert_load_error :load, "/_non_existent_"
    assert_load_error :load, "_non_existent_"
    assert_load_error :load, "./_non_existent_"
    assert_load_error :load, "~/_non_existent_"
    assert_load_error :load, "#{FIXTURE_DIR}/dir"
  end

  assert "invalid argument" do
    assert_raise(TypeError){load :def_lv}
    assert_raise(ArgumentError){load}
    assert_raise(ArgumentError){load "a", true} if mruby?
  end
end  # Kernel#load

assert "Kernel#require" do
  assert "absolute path with extension" do
    $loaded_paths.clear
    load_paths!([nil]) do  # should not read $LOAD_PATH
      path = "#{FIXTURE_DIR}/def_class.rb"
      assert_true require(path)
      assert_equal 1, TestRequireInLoadedFile.m
      assert_equal [path], $loaded_paths
    end
  end

  assert "absolute path without extension: should complete extension" do
    $loaded_paths.clear
    load_paths!([nil]) do  # should not read $LOAD_PATH
      path = "#{FIXTURE_DIR}/def_lv"
      assert_true require(path)
      assert_equal ["#{path}.rb"], $loaded_paths
    end
  end

  assert "absolute path with unkown extension: same as without extension" do
    $loaded_paths.clear
    $loaded_key = nil
    load_paths([nil]) do  # should not read $LOAD_PATH
      path = "#{FIXTURE_DIR}/key.ruby"
      assert_true require(path)
      assert_equal "key.ruby.rb", $loaded_key
      assert_equal ["#{path}.rb"], $loaded_paths
    end
  end

  assert "start with '~' with extension" do
    $loaded_paths.clear
    load_paths!([nil]) do  # should not read $LOAD_PATH
      chhome("#{FIXTURE_DIR}/link1") do |home|
        assert_true require("~/def_class.rb")
        assert_equal 1, TestRequireInLoadedFile.m
        assert_equal ["#{home}/def_class.rb"], $loaded_paths
      end
    end
  end

  assert "start with '~' without extension: should complete extension" do
    $loaded_paths.clear
    load_paths!([nil]) do  # should not read $LOAD_PATH
      chhome("#{FIXTURE_DIR}/link1") do |home|
        assert_true require("~/def_lv")
        assert_equal ["#{home}/def_lv.rb"], $loaded_paths
      end
    end
  end

  assert "start with '~' with unkown extension: same as without extension" do
    $loaded_paths.clear
    $loaded_key = nil
    load_paths([nil]) do  # should not read $LOAD_PATH
      chhome("#{FIXTURE_DIR}/link1") do |home|
        assert_true require("~/key.ruby")
        assert_equal "key.ruby.rb", $loaded_key
        assert_equal ["#{home}/key.ruby.rb"], $loaded_paths
      end
    end
  end

  assert "explicit relative path should use current dir as base dir" do
    $loaded_paths.clear
    load_paths!([nil]) do  # should not read $LOAD_PATH
      chdir("#{FIXTURE_DIR}/dir") do
        assert_true require("../link2/def_lv")
        assert_equal ["#{FIXTURE_DIR}/link2/def_lv.rb"], $loaded_paths
      end
    end
  end

  assert "implicit relative path should use $LOAD_PATH" do
    $loaded_paths.clear
    $loaded_key = nil
    load_paths!(["/_non_existent_", "#{FIXTURE_DIR}/dir", FIXTURE_DIR, nil]) do
      feature = "key.ruby"
      assert_true require(feature)
      assert_equal "dir/#{feature}.rb", $loaded_key
      assert_equal ["#{FIXTURE_DIR}/dir/#{feature}.rb"], $loaded_paths
    end
  end

  assert "implicit relative path with extension" do
    $loaded_paths.clear
    $loaded_key = nil
    load_paths!(["/_non_existent_", "#{FIXTURE_DIR}/dir", FIXTURE_DIR, nil]) do
      feature = "foo.rb"
      assert_true require(feature)
      assert_equal "dir/#{feature}", $loaded_key
      assert_equal ["#{FIXTURE_DIR}/dir/#{feature}"], $loaded_paths
    end
  end

  assert "$LOAD_PATH does not include current directory implicitly" do
    $loaded_paths.clear
    load_paths([]) do
      chdir(FIXTURE_DIR) do
        assert_load_error :require, "def_class"
      end
    end
  end

  assert "should not load same implicitly relative features" do
    $loaded_paths.clear
    load_paths([FIXTURE_DIR]) do
      feature = "same_feat"
      exp_loaded_paths = ["#{FIXTURE_DIR}/#{feature}.rb"]
      assert_true require(feature)
      assert_equal exp_loaded_paths, $loaded_paths
      $LOAD_PATH.unshift "#{FIXTURE_DIR}/dir"
      assert_false require(feature)
      assert_equal exp_loaded_paths, $loaded_paths
    end
  end

  assert "should not load same implicitly relative features with extension" do
    $loaded_paths.clear
    load_paths([FIXTURE_DIR]) do
      feature = "same_feat_with_ext"
      exp_loaded_paths = ["#{FIXTURE_DIR}/#{feature}.rb"]
      assert_true require(feature)
      assert_equal exp_loaded_paths, $loaded_paths
      assert_false require("#{feature}.rb")
      assert_equal exp_loaded_paths, $loaded_paths
    end
  end

  assert "should not load same path" do
    $loaded_paths.clear
    load_paths([FIXTURE_DIR]) do
      feature = "same_path"
      exp_loaded_paths = ["#{FIXTURE_DIR}/#{feature}.rb"]
      assert_true require(exp_loaded_paths[0])
      chhome(FIXTURE_DIR) do
        assert_false require("~/#{feature}")
        assert_equal exp_loaded_paths, $loaded_paths
      end
      chdir(FIXTURE_DIR) do
        assert_false require("./#{feature}")
        assert_equal exp_loaded_paths, $loaded_paths
      end
      assert_false require(feature)
      assert_equal exp_loaded_paths, $loaded_paths
    end
  end

  # TODO: require in require. 循環も。

  assert "environment (#4931)" do
    load_paths([FIXTURE_DIR]) do
      assert_true require("env1")
      assert_true require("env2")
      assert_equal [1, -2, 3], $environment_called
      assert_equal [5, 6], $environment_lvars
    end
  end

  assert "load error" do
    assert_load_error :require, "/_non_existent_"
    assert_load_error :require, "_non_existent_"
    assert_load_error :require, "./_non_existent_"
    assert_load_error :require, "~/_non_existent_"
    assert_load_error :require, "#{FIXTURE_DIR}/dir"
  end

  assert "invalid argument" do
    assert_raise(TypeError){require :def_lv}
    assert_raise(ArgumentError){require}
    assert_raise(ArgumentError){require "a", true}
  end

  assert "invalid value in $LOAD_PATH" do
    load_paths([nil]) do
      assert_raise(TypeError){require "_non_existent_"}
    end
    load_paths(["a\0b"]) do
      assert_raise_with_message_pattern(ArgumentError, "*null byte*") do
        require "_non_existent_"
      end
    end
  end

  assert "invalid type of $LOAD_PATH" do
    org_paths, $LOAD_PATH = $LOAD_PATH, {}
    begin
      assert_raise(TypeError){require "def_lv"}
    ensure
      $LOAD_PATH = org_paths
    end
  end if mruby?

end  # Kernel#require
