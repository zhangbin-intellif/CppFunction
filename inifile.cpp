#include "stringutil.h"
#include "inifile.h"
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <String.h>
namespace inifile{
int INI_BUF_SIZE=2048;

IniFile::IniFile()
{
    flags_.push_back("#");
    flags_.push_back(";");
}

bool IniFile::parse(const string &content,string &key,string &value,string & comment, char c/*= '='*/)
{
    size_t i = 0;
    size_t len = content.length();

    while(i < len && content[i] != c)
    {
        ++i;
    }
    
    if(i >= 0 && i < len)
    {
        key = string(content.c_str(),i);
        trim(key);
        
        value = string(content.c_str()+i+1,len-i-1);

        size_t valuelen = value.length();

        i = 0;

        while(i < valuelen)
        {
            if(value[i] == ';' || value[i] == '#')
            {
                break;
            }

            i++;
        }

        if(i < valuelen)
        {
            if((i + 1) < valuelen)
            {
                comment += string(value.c_str(),i + 1, valuelen - 1) + delim; 
            }

            value = string(value.c_str(),i);
        }
        
        
        trim(value);
        
        return true;
    }

    return false;
}

int IniFile::openini(const string & strIniData, int dwIniType)
{   
    release();
    
    IniSection * section = NULL;

    std::stringstream ssIniStream;

    string line;
    string comment;
    
    section       = new IniSection();
    sections_[""] = section;

    if(dwIniType == IFACE_INI_PARAM_TYPE_NAME)
    {
        fname_               = strIniData;
    
        std::ifstream ifStream(strIniData);

        if(!ifStream.is_open())
        {
            return -1;
        }

        ssIniStream << ifStream.rdbuf();
    }
    else
    {
        fname_  = "defini.ini";
        
        ssIniStream << strIniData;
    }
    
    while(getline(ssIniStream, line))
    {
        
        trimright(line,'\n');
        trimright(line,'\r');
        trim(line);
        
        if(line.length() <= 0)
        {
            continue;
        }

        if(line[0] == '[')
        {
            section = NULL;
            int index = (int)line.find_first_of(']');

            if(index == -1)
            {
                fprintf(stderr,"no match\n");
                return -1;
            }
            int len = index-1;
            if(len <= 0)
            {
                fprintf(stderr,"segment empty\n");
                continue;
            }
            string s(line,1,len);

            if(getSection(s.c_str()) != NULL)
            {
                fprintf(stderr,"segment duplicated:%s\n",s.c_str());
                return -1;
            }
            
            section = new IniSection();
            sections_[s] = section;

            section->name = s;

            if((index + 1) < (int)line.length())
            {
                string sectionRight(line,index + 1, line.length()-1);
                trim(sectionRight);
                
                if(isComment(sectionRight))
                {
                    comment += sectionRight + delim;
                }
            }
            
            section->comment = comment;
            comment = "";
        }
        else if(isComment(line))
        {
            comment +=  line + delim;
        }
        else
        {
            string key,value;
            if(parse(line,key,value,comment))
            {
                IniItem item;
                item.key = key;
                item.value = value;
                item.comment = comment;

                section->items.push_back(item);
            }
            else
            {
                fprintf(stderr,"parameter error[%s]\n",line.c_str());
            }
            comment = "";
        }

    }
    
    return 0;
}

int IniFile::save()
{
    return saveas(fname_);
}

int IniFile::saveas(const string &filename)
{
    string data = "";
    for(iterator sect = sections_.begin(); sect != sections_.end(); ++sect)
    {
        if(sect->second->comment != "")
        {
            data += sect->second->comment;  
            data += delim;
        }
        if(sect->first != "")
        {
            data += string("[")+sect->first + string("]");  
            data += delim;
        }

        for(IniSection::iterator item = sect->second->items.begin(); item != sect->second->items.end(); ++item)
        {
            if(item->comment != "")
            {
                data += item->comment;  
                data += delim;
            }
            data += item->key+"="+item->value;
            data += delim;
        }
    }

    FILE *fp = fopen(filename.c_str(),"w");

    fwrite(data.c_str(),1,data.length(),fp);

    fclose(fp);
    
    return 0;
}

IniSection *IniFile::getSection(const string &section /*=""*/)
{
    iterator it = sections_.find(section);
    if(it != sections_.end())
    {
        return it->second;
    }

    return NULL;
}

string IniFile::getStringValue(const string &section,const string &key,int &ret)
{
    string value,comment;
    ret = getValue(section,key,value,comment);
    return value;
}

int64_t IniFile::getIntValue(const string &section,const string &key,int &ret)
{
    string value,comment;
    ret = getValue(section,key,value,comment);
    return _atoi64(value.c_str());
}

double IniFile::getDoubleValue(const string &section,const string &key,int &ret)
{
    string value,comment;
    ret = getValue(section,key,value,comment);
    return atof(value.c_str());
}

int IniFile::getValue(const string &section,const string &key,string &value)
{
    string comment;
    return getValue(section,key,value,comment);
}

int IniFile::getValue(const string &section,const string &key,string &value,string &comment)
{
    IniSection * sect = getSection(section);
    if(sect != NULL)
    {
        for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it){
            if(it->key == key)
            {
                value = it->value;
                comment = it->comment;
                return RET_OK;
            }
        }
    }
    return RET_ERR;
}

int IniFile::getValues(const string &section,const string &key,vector<string> &values)
{
    vector<string> comments;
    return getValues(section,key,values,comments);
}

int IniFile::getValues(const string &section,const string &key,vector<string> &values,vector<string> &comments)
{
    string value,comment;

    values.clear();
    comments.clear();

    IniSection * sect = getSection(section);

    if(sect != NULL)
    {
        for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it)
        {
            if(it->key == key)
            {
                value = it->value;
                comment = it->comment;
                
                values.push_back(value);
                comments.push_back(comment);
            }
        }
    }

    return (values.size() ? RET_OK : RET_ERR);
}

bool IniFile::hasSection(const string &section) 
{
    return (getSection(section) != NULL);
}

bool IniFile::hasKey(const string &section,const string &key)
{
    IniSection * sect = getSection(section);

    if(sect != NULL)
    {
        for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it)
        {
            if(it->key == key)
            {
                return true;
            }
        }
    }
    return false;
}

int IniFile::getSectionComment(const string &section,string & comment)
{
    comment = "";
    IniSection * sect = getSection(section);
    
    if(sect != NULL)
    {
        comment = sect->comment;
        return RET_OK;
    }

    return RET_ERR;
}

int IniFile::setSectionComment(const string &section,const string & comment)
{
    IniSection * sect = getSection(section);
    
    if(sect != NULL)
    {
        sect->comment = comment;
        return RET_OK;
    }

    return RET_ERR;
}

int IniFile::setValue(const string &section,const string &key, const string &value,const string &comment /*=""*/)
{
    IniSection * sect = getSection(section);
    
    string comt = comment;
    if (comt != "")
    {
        comt = flags_[0] +comt;
    } 
    if(sect == NULL)
    {
        sect = new IniSection();
        if(sect == NULL)
        {
            fprintf(stderr,"no enough memory!\n");
            exit(-1);
        }
        sect->name = section;
        sections_[section] = sect;
    }
    
    for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it)
    {
        if(it->key == key)
        {
            it->value = value;
            it->comment = comt;
            return RET_OK;
        }
    }

    //not found key
    IniItem item;
    item.key = key;
    item.value = value;
    item.comment = comt;

    sect->items.push_back(item);

    return RET_OK;

}

int IniFile::setIntValue(const string &section, const string &key,
	const int &value, const string &comment /*=""*/)
{
	std::ostringstream strValue;
	strValue << value;
	return setValue(section, key, strValue.str(), comment);
}

int IniFile::setDoubleValue(const string &section, const string &key, const double &value, const string &comment /*=""*/)
{
	std::ostringstream strValue;
	strValue << value;
	return setValue(section, key, strValue.str(), comment);

}

void IniFile::getCommentFlags(vector<string> &flags)
{
    flags = flags_;
}

void IniFile::setCommentFlags(const vector<string> &flags)
{
    flags_ = flags;
}

void IniFile::deleteSection(const string &section)
{
    IniSection *sect = getSection(section);

    if(sect != NULL)
    {
    
        sections_.erase(section);   
        delete sect;
    }
}

void IniFile::deleteKey(const string &section,const string &key)
{
    IniSection * sect = getSection(section);
    
    if(sect != NULL)
    {
        for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it)
        {
            if(it->key == key)
            {
                sect->items.erase(it);
                break;
            }
        }
    }
}

void IniFile::release()
{
    fname_ = "";

    for(iterator i = sections_.begin(); i != sections_.end(); ++i)
    {
        delete i->second;
    }

    sections_.clear();

}

bool IniFile::isComment(const string &str)
{
    bool ret =false;
    for(unsigned int i = 0; i < flags_.size(); ++i)
    {
        unsigned int k = 0;
        if(str.length() < flags_[i].length())
        {
            continue;
        }
        for(k = 0;k < flags_[i].length(); ++k)
        {
            if(str[k] != flags_[i][k])
            {
                break;
            }
        }

        if(k == flags_[i].length())
        {
            ret = true;
            break;
        }
    }

    return ret;
}
//for debug
void IniFile::print()
{
    printf("filename:[%s]\n",fname_.c_str());

    printf("flags_:[");
    for(unsigned int i = 0; i < flags_.size(); ++i)
    {
        printf(" %s ",flags_[i].c_str());
    }
    printf("]\n");

    for(iterator it = sections_.begin(); it != sections_.end(); ++it)
    {
        printf("section:[%s]\n",it->first.c_str());
        printf("comment:[%s]\n",it->second->comment.c_str());
        for(IniSection::iterator i = it->second->items.begin(); i != it->second->items.end(); ++i)
        {
            printf("    comment:%s\n",i->comment.c_str());
            printf("    parm   :%s=%s\n",i->key.c_str(),i->value.c_str());
        }
    }
}
}
