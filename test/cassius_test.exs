defmodule CassiusTest do
  use ExUnit.Case

  test "able to listen to cwd" do
    assert Cassius.watch(".", :all) == :ok
  end

  test "receive file events" do
	Cassius.watch(".", :all)
	file = 'foobar'
	System.cmd("touch", [file], [])
	System.cmd("rm", [file], [])
	receive do
		{_mask, path} -> 
			assert path == file
	end	
  end

  test "receive file events in sub dirs" do
	Cassius.watch("lib", :all)
	file = 'lib/foobar'
	System.cmd("touch", [file], [])
	System.cmd("rm", [file], [])
	receive do
		{_mask, path} -> 
			assert path == 'foobar'
	end	
  end

  test "allow multiple watchers" do
	assert [Cassius.watch(".", :all),
		Cassius.watch("lib", :open),
		Cassius.watch("test", :close)] == [:ok, :ok, :ok]
  end

  test "receive create file events" do
	Cassius.watch("lib", :create)
	file = 'lib/foobar'
	System.cmd("touch", [file], [])
	System.cmd("rm", [file], [])
	receive do
		{:create, path} -> 
			assert path == 'foobar'
	end	
  end

  test "receive open file events" do
	Cassius.watch("lib", :open)
	file = 'lib/foobar'
	System.cmd("touch", [file], [])
	System.cmd("rm", [file], [])
	receive do
		{:open, path} -> 
			assert path == 'foobar'
	end	
  end

  test "receive close file events" do
	Cassius.watch("lib", :close)
	file = 'lib/foobar'
	System.cmd("touch", [file], [])
	System.cmd("rm", [file], [])
	receive do
		{:close, path} -> 
			assert path == 'foobar'
	end	
  end

end
