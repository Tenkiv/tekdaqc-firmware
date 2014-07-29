var group__telnet__server =
[
    [ "TelnetOpts_t", "struct_telnet_opts__t.html", [
      [ "flags", "struct_telnet_opts__t.html#a639870aa0bd3049b341f02dd37e02f1f", null ],
      [ "option", "struct_telnet_opts__t.html#a454fa9de3ca009e3082488412cd184d4", null ]
    ] ],
    [ "TelnetServer_t", "struct_telnet_server__t.html", [
      [ "buffer", "struct_telnet_server__t.html#a2153306d0b7676b5fbe604eb2d5d8dd9", null ],
      [ "close", "struct_telnet_server__t.html#a65bcef02b557e4aa1e0f04cc2ae743b9", null ],
      [ "halt", "struct_telnet_server__t.html#a9a80d626b495810428f2f14b4dae6ecb", null ],
      [ "length", "struct_telnet_server__t.html#a41a73345f82ef7f47757b13967fcc815", null ],
      [ "outstanding", "struct_telnet_server__t.html#ad311f0113656983251afddd4b62e758e", null ],
      [ "pcb", "struct_telnet_server__t.html#a7d8c5f5eed3154f7848c3a2ac9a9863a", null ],
      [ "previous", "struct_telnet_server__t.html#a8616416f9e7e002fe8f0a98f0ed71fd5", null ],
      [ "recvBuffer", "struct_telnet_server__t.html#a22aa9df37278d5baef90c6738214d9a1", null ],
      [ "recvRead", "struct_telnet_server__t.html#a4b9f32d419aaecd74a928bb8286f2a7b", null ],
      [ "recvWrite", "struct_telnet_server__t.html#ac07efa3d17c62c9b958f4324b78ebf55", null ],
      [ "state", "struct_telnet_server__t.html#a8010cffb8a33a3f5d540dcd6ef57a7b6", null ]
    ] ],
    [ "CONNECTED", "group__telnet__server.html#gaf6202935c026af12978d46a765dafb90", null ],
    [ "NOT_CONNECTED", "group__telnet__server.html#ga0e9795e3bb758850785e6ebc676ab01b", null ],
    [ "OPT_FLAG_DO", "group__telnet__server.html#gaab89806cad0a31ec6d4cae073ffbebcb", null ],
    [ "OPT_FLAG_WILL", "group__telnet__server.html#ga87cde070c7f961821be5f38065c6a4a0", null ],
    [ "TELNET_BUFFER_LENGTH", "group__telnet__server.html#gae9088b6c383afeff8d58098f141afbd3", null ],
    [ "TELNET_IAC", "group__telnet__server.html#ga8b600918f84783490fd791ce773175ab", null ],
    [ "TELNET_OPT_BINARY", "group__telnet__server.html#ga9b82c3af8c3a14bc1f758d79a00e9ee9", null ],
    [ "TelnetState_t", "group__telnet__server.html#ga6a34decdb77d8c9df8ead5420ea8d326", [
      [ "STATE_NORMAL", "group__telnet__server.html#gga6a34decdb77d8c9df8ead5420ea8d326ae583c55c9f9a3aa9888e4ed771d6a9b6", null ],
      [ "STATE_IAC", "group__telnet__server.html#gga6a34decdb77d8c9df8ead5420ea8d326ac445fb392e1398d85894ed95c217535d", null ],
      [ "STATE_WILL", "group__telnet__server.html#gga6a34decdb77d8c9df8ead5420ea8d326aab00693e6446eb0b2dd86441e16fa7bd", null ],
      [ "STATE_WONT", "group__telnet__server.html#gga6a34decdb77d8c9df8ead5420ea8d326a1c98dcef9f1808d490980c9660d07356", null ],
      [ "STATE_DO", "group__telnet__server.html#gga6a34decdb77d8c9df8ead5420ea8d326a690d160db6429d82862282d94e3b4c4f", null ],
      [ "STATE_DONT", "group__telnet__server.html#gga6a34decdb77d8c9df8ead5420ea8d326a8efcfd07c99adc3da1f982af25bb6054", null ]
    ] ],
    [ "TelnetStatus_t", "group__telnet__server.html#gab6653d6c1bd1261ebfcb4d667f848716", [
      [ "TELNET_OK", "group__telnet__server.html#ggab6653d6c1bd1261ebfcb4d667f848716a63347d81d258d4f3c53dca873fbe3b41", null ],
      [ "TELNET_ERR_CONNECTED", "group__telnet__server.html#ggab6653d6c1bd1261ebfcb4d667f848716ade43926fa79e7274b5a207aa99dfbf80", null ],
      [ "TELNET_ERR_BIND", "group__telnet__server.html#ggab6653d6c1bd1261ebfcb4d667f848716a0ebd3cbc0fc21b74010e6faf772852e9", null ],
      [ "TELNET_ERR_PCBCREATE", "group__telnet__server.html#ggab6653d6c1bd1261ebfcb4d667f848716a01d38959911101e3354606780cf67a6d", null ]
    ] ],
    [ "InitializeTelnetServer", "group__telnet__server.html#gae8002482499a83baab72c8714ef609d2", null ],
    [ "TelnetClose", "group__telnet__server.html#ga26f786d2e47e4bb5f180d2740eebc957", null ],
    [ "TelnetIsConnected", "group__telnet__server.html#ga6509e0047d80aea0570ec903a1e10887", null ],
    [ "TelnetPoll", "group__telnet__server.html#ga8946efc99077a959cb1cad3e059f9071", null ],
    [ "TelnetProcessCharacter", "group__telnet__server.html#ga348f2b04116a013b6eabfa72c926a25f", null ],
    [ "TelnetProcessDo", "group__telnet__server.html#gabdf9478794e09f61ab850c78147a7c6f", null ],
    [ "TelnetProcessDont", "group__telnet__server.html#gaf2500a4ad4bcbfd6ac4367f298e42f10", null ],
    [ "TelnetProcessWill", "group__telnet__server.html#ga8779bc1d2c7943063104c8624a877d8b", null ],
    [ "TelnetProcessWont", "group__telnet__server.html#gac84ea97f4be2d0b62498cca6297d0ab7", null ],
    [ "TelnetRead", "group__telnet__server.html#gaff55b8914481cb32d621a855a69a5635", null ],
    [ "TelnetRecvBufferWrite", "group__telnet__server.html#ga93c1e298d9778f890b6df05b35072988", null ],
    [ "TelnetWrite", "group__telnet__server.html#gaa2f418e9d287a2863fe1198e3c75f4ad", null ],
    [ "TelnetWriteCommandDataMessage", "group__telnet__server.html#ga7f3d87d181908e8353a67bbaf15fac50", null ],
    [ "TelnetWriteDebugMessage", "group__telnet__server.html#ga7d350b6355ef213d4aa15e9b80c3d15e", null ],
    [ "TelnetWriteErrorMessage", "group__telnet__server.html#ga0660c325387baca6c54fe699ac048907", null ],
    [ "TelnetWriteStatusMessage", "group__telnet__server.html#ga0afb5de5ce2d81cdc5322e0534bae1d8", null ],
    [ "TelnetWriteString", "group__telnet__server.html#ga579198334cf97874ecc89e3b29351066", null ]
];