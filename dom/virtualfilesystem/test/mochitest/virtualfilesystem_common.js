
const mountOption = {fileSystemId: "dummyId",
                     displayName: "dummyFileSystem",
                     writable: false,
                     openedFilesLimit: 10};

const unmountOption = {fileSystemId: "dummyId"};

const mountOption1 = {fileSystemId: "dummyId1",
                      displayName: "dummyFileSystem1",
                      writable: true,
                      openedFilesLimit: 100};

const unmountOption1 = {fileSystemId: "dummyId1"};

const testPath = "/test/dummy.txt";

const openFileMode = "Read";

const entryMetadata = {isDirectory: true,
                       name: "test",
                       size: 999,
                       modificationTime: 100,
                       mimeType: "text/directory"};

const entryMetadata1 = {isDirectory: false,
                        name: "test1",
                        size: 100,
                        modificationTime: 1000,
                        mimeType: "text/file"};


