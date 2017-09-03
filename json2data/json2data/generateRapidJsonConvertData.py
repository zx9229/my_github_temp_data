# coding=utf-8
# -*- coding: utf-8 -*-
# // 本文件应以 UTF-8 无 BOM 格式编码.
import codecs
import re
import sys


class Gen(object):
    @staticmethod
    def LINE_DELIMITER():
        return "\r\n"

    @staticmethod
    def loadFile(filename, encoding="utf_8"):
        with codecs.open(filename=filename, mode='r', encoding=encoding) as f:
            content = f.read()
        return content

    @staticmethod
    def saveFile(filename, content, encoding="utf_8"):
        with codecs.open(filename=filename, mode='w', encoding=encoding) as f:
            f.write(content)
        return None

    @staticmethod
    def processContent(content):
        """处理原始内容,移除注释,空行"""
        # 将多行注释(/*多行注释的内容*/)替换为一个空格,需要启用最小匹配,才能正确替换.
        content_1 = re.sub(re.compile(r"/\*.*?\*/", re.DOTALL), " ", content)
        # 删除单行注释.
        content_2 = re.sub("//.*", "", content_1)
        # 合并多个行结束符.
        content_3 = re.sub("[\r\n]+", "\r\n", content_2)
        # (public:)|(private:)|(protected:)
        content_4 = re.sub(
            "([ \t]*public:[ \r\n]+)|([ \t]*private:[ \r\n]+)|([ \t]*protected:[ \r\n]+)",
            "\r\n", content_3)
        # TODO: 如果有函数的话,还需要移除函数.
        # TODO: 如果它是一个派生类,那么基类的成员暂时无法处理.
        content_finish = content_4
        return content_finish

    @staticmethod
    def calcStructData(content):
        patternStr = r"[ \t\r\n]*(class|struct)[ \t]+(?P<cName>[a-zA-Z0-9_:]+)[ \t\r\n]*\{[ \t\r\n]*(?P<cData>.*?)[ \t\r\n]*\};"
        pattern = re.compile(patternStr, re.DOTALL)
        retList = re.findall(pattern, content)
        return retList

    @staticmethod
    def calcFieldData(content):
        patternStr = r"[ \t\r\n]*(?P<fieldType>[a-zA-Z0-9_:]+)[ \t\r\n]+(?P<fieldName>[a-zA-Z0-9_]+)[ \t\r\n]*;"
        pattern = re.compile(patternStr, re.DOTALL)
        retList = re.findall(pattern, content)
        return retList

    @staticmethod
    def calcToData(oneStructData):
        cName = oneStructData[1]
        cData = oneStructData[2]
        #
        to__DataContent = ""
        to__DataContent += "void toData(rapidjson::Value& jValue, {clsName}& dataOut, int size)".format(clsName=cName) + Gen.LINE_DELIMITER()
        to__DataContent += "{" + Gen.LINE_DELIMITER()
        to__DataContent += "    dataOut = {};" + Gen.LINE_DELIMITER()
        to__DataContent += "    rapidjson::Value::MemberIterator it;" + Gen.LINE_DELIMITER()
        #
        allFieldData = Gen.calcFieldData(cData)
        for fieldData in allFieldData:
            fName = fieldData[1]
            to__DataContent += '    if (jValue.MemberEnd() != (it = jValue.FindMember("{fieldName}")))'.format(fieldName=fName) + Gen.LINE_DELIMITER()
            to__DataContent += '        toData(it->value, dataOut.{fieldName}, sizeof(dataOut.{fieldName}));'.format(fieldName=fName) + Gen.LINE_DELIMITER()
        to__DataContent += "};" + Gen.LINE_DELIMITER()
        #
        return to__DataContent

    @staticmethod
    def calcFromData(oneStructData):
        cName = oneStructData[1]
        cData = oneStructData[2]
        #
        fromDataContent = ""
        fromDataContent += "void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const {clsName}& data)".format(clsName=cName) + Gen.LINE_DELIMITER()
        fromDataContent += "{" + Gen.LINE_DELIMITER()
        fromDataContent += "    jvOut.SetObject();" + Gen.LINE_DELIMITER()
        fromDataContent += "    jvOut.RemoveAllMembers();" + Gen.LINE_DELIMITER()
        fromDataContent += "    rapidjson::Value jValue;" + Gen.LINE_DELIMITER()
        #
        allFieldData = Gen.calcFieldData(cData)
        for fieldData in allFieldData:
            fName = fieldData[1]
            fromDataContent += "    fromData(jValue, allocator, data.{fieldName});".format(fieldName=fName) + Gen.LINE_DELIMITER()
            fromDataContent += '    jvOut.AddMember("{fieldName}", jValue, allocator);'.format(fieldName=fName) + Gen.LINE_DELIMITER()
        fromDataContent += "};" + Gen.LINE_DELIMITER()
        #
        return fromDataContent

    @staticmethod
    def test(srcFilename, dstFilename, encoding="utf_8"):
        fileContent = ""
        content = Gen.loadFile(srcFilename, encoding)
        content = Gen.processContent(content)
        allStructData = Gen.calcStructData(content)
        for structData in allStructData:
            oneTo__Data = Gen.calcToData(structData)
            oneFromData = Gen.calcFromData(structData)
            fileContent += oneTo__Data
            fileContent += oneFromData
        Gen.saveFile(dstFilename, fileContent, encoding)
        return None


if __name__ == "__main__":
    if True:
        srcFilename = "AllClassDefinition.h" if len(sys.argv) <= 1 else sys.argv[1]
        dstFilename = "_json2data.h" if len(sys.argv) <= 2 else sys.argv[2]
        encoding = "utf-8" if len(sys.argv) <= 3 else sys.argv[3]
        Gen.test(srcFilename, dstFilename, encoding)
        exit(0)
    else:
        if len(sys.argv) <= 2:
            print("命令: srcFilename           dstFilename   encoding")
            print("例如: AllClassDefinition.h  _json2data.h  utf-8")
            exit(0)
        else:
            srcFilename = sys.argv[1]
            dstFilename = sys.argv[2]
            encoding = "utf-8" if len(sys.argv) <= 3 else sys.argv[3]
            Gen.test(srcFilename, dstFilename, encoding)
            exit(0)
    assert False
    exit(0)
