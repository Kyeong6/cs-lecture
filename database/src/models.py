from sqlalchemy import Column, String, DateTime, Text, ForeignKey, BigInteger, Boolean
from sqlalchemy.orm import relationship
from sqlalchemy.ext.declarative import declarative_base
from connect import engine
from sqlalchemy.orm import registry

registry = registry()
Base = declarative_base()

# COMPANY 테이블
class Company(Base):
    __tablename__ = "COMPANY"

    COMPANY_ID = Column(BigInteger, primary_key=True, autoincrement=True)
    COMPANY_NAME = Column(String(255), unique=True, nullable=False)
    COUNTRY = Column(String(255), nullable=False)
    WEBSITE_URL = Column(String(255), nullable=False)
    LOGO_URL = Column(String(255), nullable=False)
    CREATED_AT = Column(DateTime(6), nullable=False)
    UPDATED_AT = Column(DateTime(6), nullable=False)

    blogs = relationship("Blog", back_populates="company", cascade="all, delete-orphan")


# BLOG 테이블
class Blog(Base):
    __tablename__ = "BLOG"

    BLOG_ID = Column(BigInteger, primary_key=True, autoincrement=True)
    COMPANY_ID = Column(BigInteger, ForeignKey("COMPANY.COMPANY_ID", ondelete="RESTRICT"), nullable=True)
    TITLE = Column(String(255), nullable=False)
    CONTENT = Column(Text, nullable=False)
    BLOG_URL = Column(String(255), unique=True, nullable=False)
    PUBLISHED_DATE = Column(DateTime(6), nullable=False)
    IS_FOREIGN = Column(Boolean, nullable=False, default=False)
    CREATED_AT = Column(DateTime(6), nullable=False)
    UPDATED_AT = Column(DateTime(6), nullable=False)

    company = relationship("Company", back_populates="blogs")
    translations = relationship("Translation", back_populates="blog", cascade="all, delete-orphan")
    images = relationship("Image", back_populates="blog", cascade="all, delete-orphan")
    blog_tags = relationship("BlogTag", back_populates="blog", cascade="all, delete-orphan")


# TRANSLATION 테이블
class Translation(Base):
    __tablename__ = "TRANSLATION"

    BLOG_ID = Column(BigInteger, ForeignKey("BLOG.BLOG_ID", ondelete="CASCADE"), primary_key=True)
    TRANSLATED_TITLE = Column(String(255), nullable=False)
    TRANSLATED_CONTENT = Column(Text, nullable=False)
    CREATED_AT = Column(DateTime(6), nullable=False)
    UPDATED_AT = Column(DateTime(6), nullable=False)

    blog = relationship("Blog", back_populates="translations")


# IMAGE 테이블
class Image(Base):
    __tablename__ = "IMAGE"

    IMAGE_ID = Column(BigInteger, primary_key=True, autoincrement=True)
    BLOG_ID = Column(BigInteger, ForeignKey("BLOG.BLOG_ID", ondelete="CASCADE"), nullable=True)
    IMAGE_URL = Column(String(255), unique=True, nullable=False)
    CREATED_AT = Column(DateTime(6), nullable=False)
    UPDATED_AT = Column(DateTime(6), nullable=False)

    blog = relationship("Blog", back_populates="images")


# TAG 테이블
class Tag(Base):
    __tablename__ = "TAG"

    TAG_ID = Column(BigInteger, primary_key=True, autoincrement=True)
    TAG_TYPE = Column(String(255), nullable=False)
    CREATED_AT = Column(DateTime(6), nullable=False)
    UPDATED_AT = Column(DateTime(6), nullable=False)

    blog_tags = relationship("BlogTag", back_populates="tag", cascade="all, delete-orphan")


# BLOG_TAG 테이블
class BlogTag(Base):
    __tablename__ = "BLOG_TAG"

    BLOG_TAG_ID = Column(BigInteger, primary_key=True, autoincrement=True)
    BLOG_ID = Column(BigInteger, ForeignKey("BLOG.BLOG_ID", ondelete="CASCADE"), nullable=True)
    TAG_ID = Column(BigInteger, ForeignKey("TAG.TAG_ID", ondelete="CASCADE"), nullable=True)
    CREATED_AT = Column(DateTime(6), nullable=False)
    UPDATED_AT = Column(DateTime(6), nullable=False)

    blog = relationship("Blog", back_populates="blog_tags")
    tag = relationship("Tag", back_populates="blog_tags")


# USER 테이블
class User(Base):
    __tablename__ = "USER"

    USER_ID = Column(BigInteger, primary_key=True, autoincrement=True)
    USER_NAME = Column(String(255), unique=True, nullable=False)
    EMAIL = Column(String(255), unique=True, nullable=False)
    IS_AGREED = Column(Boolean, nullable=False, default=False)
    IS_SUBSCRIBED = Column(Boolean, nullable=False, default=False)
    CREATED_AT = Column(DateTime(6), nullable=False)
    UPDATED_AT = Column(DateTime(6), nullable=False)


# 테이블 생성
Base.metadata.create_all(bind=engine)