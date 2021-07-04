const fs = require('graceful-fs')
const Base = require('lowdb/adapters/Base')

const readFile = fs.readFileSync
const writeFile = fs.writeFileSync

// Same code as in FileAsync, minus `await`
class FileSyncCallback extends Base {
  read() {
    // fs.exists is deprecated but not fs.existsSync
    if (fs.existsSync(this.source)) {
      // Read database
      try {
        const data = readFile(this.source, 'utf-8').trim()
        // Handle blank file
        return data ? this.deserialize(data) : this.defaultValue
      } catch (e) {
        if (e instanceof SyntaxError) {
          e.message = `Malformed JSON in file: ${this.source}\n${e.message}`
        }
        throw e
      }
    } else {
      // Initialize
      writeFile(this.source, this.serialize(this.defaultValue))
      return this.defaultValue
    }
  }

  write(data) {
    return writeFile(this.source, this.serialize(data))
  }

  setCallbacks(writeCb,readCb) {
    console.log('in setCallbacks');
  }
}

module.exports = FileSyncCallback