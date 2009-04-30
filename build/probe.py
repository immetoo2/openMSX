# $Id$
# Replacement for autoconf.
# Performs some test compiles, to check for headers and functions.
# It does not execute anything it builds, making it friendly for cross compiles.

from compilers import CompileCommand, LinkCommand
from makeutils import extractMakeVariables
from outpututils import rewriteIfChanged
from probe_defs import librariesByName
from probe_results import iterProbeResults

from msysutils import msysActive
from os import environ, makedirs, remove
from os.path import isdir, isfile
from shlex import split as shsplit
from subprocess import PIPE, Popen
import sys

class SystemFunction(object):
	name = None

	@classmethod
	def getFunctionName(cls):
		return cls.name

	@classmethod
	def getMakeName(cls):
		return cls.name.upper()

	@classmethod
	def iterHeaders(cls, targetPlatform):
		raise NotImplementedError

class FTruncateFunction(SystemFunction):
	name = 'ftruncate'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		yield '<unistd.h>'

class GetTimeOfDayFunction(SystemFunction):
	name = 'gettimeofday'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		yield '<sys/time.h>'

class MMapFunction(SystemFunction):
	name = 'mmap'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		if targetPlatform in ('darwin', 'openbsd'):
			yield '<sys/types.h>'
		yield '<sys/mman.h>'

class PosixMemAlignFunction(SystemFunction):
	name = 'posix_memalign'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		yield '<stdlib.h>'

class USleepFunction(SystemFunction):
	name = 'usleep'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		yield '<unistd.h>'

# Build a list of system functions using introspection.
def iterSystemFunctions(localObjects):
	for obj in localObjects:
		if isinstance(obj, type) and issubclass(obj, SystemFunction):
			if obj is not SystemFunction:
				yield obj
systemFunctions = list(iterSystemFunctions(locals().itervalues()))

def resolve(log, expr):
	# TODO: Since for example "sdl-config" is used in more than one
	#       CFLAGS definition, it will be executed multiple times.
	try:
		return normalizeWhitespace(evaluateBackticks(log, expr))
	except IOError:
		# Executing a lib-config script is expected to fail if the
		# script is not installed.
		# TODO: Report this explicitly in the probe results table.
		return ''

def writeFile(path, lines):
	out = open(path, 'w')
	try:
		for line in lines:
			print >> out, line
	finally:
		out.close()

def tryCompile(log, compileCommand, sourcePath, lines):
	'''Write the program defined by "lines" to a text file specified
	by "path" and try to compile it.
	Returns True iff compilation succeeded.
	'''
	assert sourcePath.endswith('.cc')
	objectPath = sourcePath[ : -3] + '.o'
	writeFile(sourcePath, lines)
	try:
		return compileCommand.compile(log, sourcePath, objectPath)
	finally:
		remove(sourcePath)
		if isfile(objectPath):
			remove(objectPath)

def tryLink(log, compileCommand, linkCommand, sourcePath):
	assert sourcePath.endswith('.cc')
	objectPath = sourcePath[ : -3] + '.o'
	binaryPath = sourcePath[ : -3] + '.bin'
	def dummyProgram():
		# Try to link dummy program to the library.
		# TODO: Use object file instead of dummy program.
		#       That way, object file can refer to symbol from the lib and
		#       we get more useful results from static libs.
		yield 'int main(int argc, char **argv) { return 0; }'
	writeFile(sourcePath, dummyProgram())
	try:
		compileOK = compileCommand.compile(log, sourcePath, objectPath)
		if not compileOK:
			print >> log, 'cannot test linking because compile failed'
			return False
		return linkCommand.link(log, [ objectPath ], binaryPath)
	finally:
		remove(sourcePath)
		if isfile(objectPath):
			remove(objectPath)
		if isfile(binaryPath):
			remove(binaryPath)

def checkCompiler(log, compileCommand, outDir):
	'''Checks whether compiler can compile anything at all.
	Returns True iff the compiler works.
	'''
	def hello():
		# The most famous program.
		yield '#include <iostream>'
		yield 'int main(int argc, char** argv) {'
		yield '  std::cout << "Hello World!" << std::endl;'
		yield '  return 0;'
		yield '}'
	return tryCompile(log, compileCommand, outDir + '/hello.cc', hello())

def checkFunc(log, compileCommand, outDir, checkName, funcName, headers):
	'''Checks whether the given function is declared by the given headers.
	Returns True iff the function is declared.
	'''
	def takeFuncAddr():
		# Try to include the necessary headers and get the function address.
		for header in headers:
			yield '#include %s' % header
		yield 'void (*f)() = reinterpret_cast<void (*)()>(%s);' % funcName
	return tryCompile(
		log, compileCommand, outDir + '/' + checkName + '.cc', takeFuncAddr()
		)

def checkLib(log, compileCommand, linkCommand, outDir, makeName):
	'''Checks whether the given library can be linked against.
	Returns True iff the library is available.
	'''
	return tryLink(
		log, compileCommand, linkCommand, outDir + '/' + makeName + '.cc'
		)

def iterProbeHeader(probeVars):
	yield '// Automatically generated by build system.'
	for name, value in sorted(probeVars.iteritems()):
		if name.startswith('HAVE_'):
			if value:
				yield '#define %s 1' % name
			else:
				yield '// #undef %s' % name

def backtick(log, commandLine):
	'''Run a command and capture what it writes to stdout.
	If the command fails or writes something to stderr, that is logged.
	Returns the captured string, or None if the command failed.
	'''
	# TODO: This is a modified copy-paste from compilers._Command.
	commandParts = shsplit(commandLine)
	env = dict(environ)
	while commandParts:
		if '=' in commandParts[0]:
			name, value = commandParts[0].split('=', 1)
			del commandParts[0]
			env[name] = value
		else:
			break
	else:
		raise ValueError(
			'No command specified in "%s"' % commandLine
			)

	if msysActive():
		commandParts = [ environ['SHELL'], '-c', shjoin(commandParts) ]

	try:
		proc = Popen(
			commandParts, bufsize = -1, env = env,
			stdin = None, stdout = PIPE, stderr = PIPE,
			)
	except OSError, ex:
		print >> log, 'Failed to execute "%s": %s' % (commandLine, ex)
		return None
	stdoutdata, stderrdata = proc.communicate()
	if stderrdata:
		severity = 'warning' if proc.returncode == 0 else 'error'
		log.write('%s executing "%s"\n' % (severity.capitalize(), commandLine))
		# pylint 0.18.0 somehow thinks stdoutdata is a list, not a string.
		# pylint: disable-msg=E1103
		stderrdata = stderrdata.replace('\r', '')
		log.write(stderrdata)
		if not stderrdata.endswith('\n'):
			log.write('\n')
	if proc.returncode == 0:
		return stdoutdata
	else:
		print >> log, 'Execution failed with exit code %d' % proc.returncode
		return None

def evaluateBackticks(log, expression):
	parts = []
	index = 0
	while True:
		start = expression.find('`', index)
		if start == -1:
			parts.append(expression[index : ])
			break
		end = expression.find('`', start + 1)
		if end == -1:
			raise ValueError('Unmatched backtick: %s' % expression)
		parts.append(expression[index : start])
		command = expression[start + 1 : end].strip()
		result = backtick(log, command)
		if result is None:
			raise IOError('Backtick evaluation failed; see log')
		parts.append(result)
		index = end + 1
	return ''.join(parts)

def shjoin(parts):
	def escape(part):
		return ''.join(
			'\\' + ch if ch in '\\ \'"$()[]' else ch
			for ch in part
			)
	return ' '.join(escape(part) for part in parts)

def normalizeWhitespace(expression):
	return shjoin(shsplit(expression))

class TargetSystem(object):

	def __init__(
		self,
		log, compileCommandStr, outDir, platform, probeVars, customVars,
		disabledLibraries, disabledFuncs, disabledHeaders
		):
		'''Create empty log and result files.
		'''
		self.log = log
		self.compileCommandStr = compileCommandStr
		self.outDir = outDir
		self.platform = platform
		self.probeVars = probeVars
		self.customVars = customVars
		self.disabledLibraries = disabledLibraries
		self.disabledFuncs = disabledFuncs
		self.disabledHeaders = disabledHeaders
		self.outMakePath = outDir + '/probed_defs.mk'
		self.outHeaderPath = outDir + '/probed_defs.hh'
		self.outVars = {}

	def checkAll(self):
		'''Run all probes.
		'''
		self.hello()
		for func in systemFunctions:
			if func.name in self.disabledFuncs:
				self.disabledFunc(func)
			else:
				self.checkFunc(func)
		for header in sorted(librariesByName.iterkeys()) + \
				[ 'GL_GL', 'GL_GLEW' ]:
			if header in self.disabledHeaders:
				self.disabledHeader(header)
			else:
				self.checkHeader(header)
		for library in sorted(librariesByName.iterkeys()):
			if library in self.disabledLibraries:
				self.disabledLibrary(library)
			else:
				self.checkLib(library)

	def printAll(self):
		def iterVars():
			yield '# Automatically generated by build system.'
			yield '# Non-empty value means found, empty means not found.'
			for item in sorted(self.outVars.iteritems()):
				yield '%s:=%s' % item
		rewriteIfChanged(self.outMakePath, iterVars())

	def printResults(self):
		for line in iterProbeResults(
			self.outVars, self.customVars,
			self.disabledHeaders, self.disabledLibraries
			):
			print line

	def everything(self):
		self.checkAll()
		self.printAll()
		rewriteIfChanged(self.outHeaderPath, iterProbeHeader(self.outVars))
		self.printResults()

	def hello(self):
		'''Check compiler with the most famous program.
		'''
		compileCommand = CompileCommand.fromLine(self.compileCommandStr, '')
		ok = checkCompiler(self.log, compileCommand, self.outDir)
		print >> self.log, 'Compiler %s: %s' % (
			'works' if ok else 'broken',
			compileCommand
			)
		self.outVars['COMPILER'] = str(ok).lower()

	def checkFunc(self, func):
		'''Probe for function.
		'''
		compileCommand = CompileCommand.fromLine(self.compileCommandStr, '')
		ok = checkFunc(
			self.log, compileCommand, self.outDir,
			func.name, func.getFunctionName(), func.iterHeaders(self.platform)
			)
		print >> self.log, '%s function: %s' % (
			'Found' if ok else 'Missing',
			func.getFunctionName()
			)
		self.outVars['HAVE_%s' % func.getMakeName()] = 'true' if ok else ''

	def checkHeader(self, makeName):
		'''Probe for header.
		'''
		flags = resolve(self.log, self.probeVars['%s_CFLAGS' % makeName])
		compileCommand = CompileCommand.fromLine(self.compileCommandStr, flags)
		headers = [ self.probeVars['%s_HEADER' % makeName] ]
		funcName = self.probeVars['%s_FUNCTION' % makeName]

		ok = checkFunc(
			self.log, compileCommand, self.outDir, makeName, funcName, headers
			)
		print >> self.log, '%s header: %s' % (
			'Found' if ok else 'Missing',
			makeName
			)
		self.outVars['HAVE_%s_H' % makeName] = 'true' if ok else ''
		self.outVars['%s_CFLAGS' % makeName] = flags

	def checkLib(self, makeName):
		'''Probe for library.
		'''
		flags = resolve(self.log, self.probeVars['%s_LDFLAGS' % makeName])
		compileCommand = CompileCommand.fromLine(self.compileCommandStr, '')
		linkCommand = LinkCommand.fromLine(self.compileCommandStr, flags)

		ok = checkLib(
			self.log, compileCommand, linkCommand, self.outDir, makeName
			)
		print >> self.log, '%s library: %s' % (
			'Found' if ok else 'Missing',
			makeName
			)
		if ok:
			result = resolve(self.log, self.probeVars['%s_RESULT' % makeName])
		else:
			result = ''
		self.outVars['HAVE_%s_LIB' % makeName] = result
		self.outVars['%s_LDFLAGS' % makeName] = flags

	def disabledFunc(self, func):
		print >> self.log, 'Disabled function: %s' % func.getFunctionName()
		self.outVars['HAVE_%s' % func.getMakeName()] = ''

	def disabledHeader(self, header):
		print >> self.log, 'Disabled header: %s' % header
		self.outVars['HAVE_%s_H' % header] = ''

	def disabledLibrary(self, library):
		print >> self.log, 'Disabled library: %s' % library
		self.outVars['HAVE_%s_LIB' % library] = ''

def main(compileCommandStr, outDir, platform, linkMode, thirdPartyInstall):
	if not isdir(outDir):
		makedirs(outDir)
	log = open(outDir + '/probe.log', 'w')
	print 'Probing target system...'
	print >> log, 'Probing system:'
	try:
		customVars = extractMakeVariables('build/custom.mk')
		disabledLibraries = set(customVars['DISABLED_LIBRARIES'].split())
		disabledFuncs = set()
		disabledHeaders = set()

		if linkMode.startswith('3RD_'):
			# Disable Jack: The CassetteJack feature is not useful for most end
			# users, so do not include Jack in the binary distribution of
			# openMSX.
			disabledLibraries.add('JACK')

			# GLEW header can be <GL/glew.h> or just <glew.h>; the dedicated
			# version we use resides in the "GL" dir, so don't look for the
			# other one, or we might pick up a different version somewhere on
			# the system.
			disabledHeaders.add('GLEW')

		disabledHeaders |= disabledLibraries
		if 'GL' in disabledLibraries:
			disabledHeaders.add('GL_GL')
		if 'GLEW' in disabledLibraries:
			disabledHeaders.add('GL_GLEW')

		distroRoot = thirdPartyInstall or None

		probeVars = {}
		for name, library in sorted(librariesByName.iteritems()):
			if name in disabledLibraries:
				continue

			header = library.header
			if hasattr(header, '__iter__'):
				header, altheader = header
			else:
				altheader = None

			probeVars['%s_HEADER' % name] = header
			probeVars['%s_FUNCTION' % name] = library.function
			if altheader is not None:
				probeVars['GL_%s_HEADER' % name] = altheader
				probeVars['GL_%s_FUNCTION' % name] = library.function

			probeVars['%s_CFLAGS' % name] = \
				library.getCompileFlags(platform, linkMode, distroRoot)
			if altheader is not None:
				probeVars['GL_%s_CFLAGS' % name] = probeVars['%s_CFLAGS' % name]
			probeVars['%s_LDFLAGS' % name] = \
				library.getLinkFlags(platform, linkMode, distroRoot)
			probeVars['%s_RESULT' % name] = \
				library.getResult(platform, linkMode, distroRoot)

		TargetSystem(
			log, compileCommandStr, outDir, platform, probeVars, customVars,
			disabledLibraries, disabledFuncs, disabledHeaders
			).everything()
	finally:
		log.close()

if __name__ == '__main__':
	if len(sys.argv) == 6:
		try:
			main(*sys.argv[1 : ])
		except ValueError, ve:
			print >> sys.stderr, ve
			sys.exit(2)
	else:
		print >> sys.stderr, (
			'Usage: python probe.py '
			'COMPILE OUTDIR OPENMSX_TARGET_OS LINK_MODE 3RDPARTY_INSTALL_DIR'
			)
		sys.exit(2)
