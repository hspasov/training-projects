const nodeExternals = require('webpack-node-externals');
const ProgressBarPlugin = require('progress-bar-webpack-plugin');

module.exports = {
  entry: {
    'server-bundled': './app.js'
  },
  externals: nodeExternals(),
  output: {
    path: __dirname,
    filename: '[name].js',
    publicPath: '/js',
    libraryTarget: 'commonjs2'
  },
  module: {
    rules: [
      {
        test: /\.js$|\.jsx$/,
        exclude: /node_modules/,
        use: [
          {
            loader: 'babel-loader',
            options: {
              presets: ['env', 'react']
            }
          }
        ]
      }
    ]
  },
  plugins: [
    new ProgressBarPlugin()
  ],
  target: 'node',
  watch: true,
  watchOptions: {
    ignored: /node_modules/
  }
};
