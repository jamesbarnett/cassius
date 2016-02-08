defmodule Cassius.Mixfile do
  use Mix.Project

  def project do
    [app: :cassius,
     version: "0.0.1",
     elixir: "~> 1.2.0",
     deps: deps]
  end

  def application do
    [applications: [:logger]]
  end

  defp deps do
    []
  end
end
