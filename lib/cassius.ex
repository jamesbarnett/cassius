defmodule Cassius do
@on_load{:init, 0}

  def init do
    :ok = :erlang.load_nif('src/linux_nif', 0)
  end
  
  def watch(dir, event) do
    _watch_(dir, event)
  end
  
  def _watch_(_, _) do end
end
